#include "ui/widgets/AssetInfoPanel.h"

#include "ui/IconUtils.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QShowEvent>
#include <QStyle>
#include <QVBoxLayout>

namespace Aurora {
namespace {

constexpr int kAvatarSize = 48;
constexpr int kLinkWidth = 220; // What fits beside the section icon in the 300 px panel.

QString cameraName(const ImmichAssetDetails &details)
{
    // Many cameras repeat the make in the model ("Canon" + "Canon EOS R5").
    if (details.model.startsWith(details.make, Qt::CaseInsensitive))
        return details.model;
    return QStringLiteral("%1 %2").arg(details.make, details.model).trimmed();
}

QString exposureSummary(const ImmichAssetDetails &details)
{
    QStringList parts;
    if (details.fNumber > 0)
        parts << QStringLiteral("ƒ/%1").arg(QLocale().toString(details.fNumber, 'g', 3));
    if (!details.exposureTime.isEmpty())
        parts << QStringLiteral("%1 s").arg(details.exposureTime);
    if (details.iso > 0)
        parts << QStringLiteral("ISO %1").arg(details.iso);
    if (details.focalLength > 0)
        parts << QStringLiteral("%1 mm").arg(QLocale().toString(details.focalLength, 'g', 4));
    return parts.join(QStringLiteral(" · "));
}

} // namespace

AssetInfoPanel::AssetInfoPanel(ImmichClient *client, QWidget *parent)
    : QFrame(parent)
    , m_client(client)
    , m_content(new QVBoxLayout)
    , m_status(new QLabel(this))
{
    setObjectName(QStringLiteral("assetInfoPanel"));
    setFixedWidth(300);

    auto *title = new QLabel(tr("Info"), this);
    title->setProperty("section", true);
    auto *closeButton = new QPushButton(this);
    closeButton->setProperty("zoomButton", true);
    closeButton->setCursor(Qt::PointingHandCursor);
    configureActionButton(closeButton, QStringLiteral(":/icons/x.svg"));
    closeButton->setToolTip(tr("Hide info (I)"));

    auto *header = new QHBoxLayout;
    header->addWidget(title, 1);
    header->addWidget(closeButton);

    auto *body = new QWidget;
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    m_content->setContentsMargins(0, 0, 0, 0);
    m_content->setSpacing(16);
    m_status->setProperty("subheading", true);
    m_status->setWordWrap(true);
    bodyLayout->addWidget(m_status);
    bodyLayout->addLayout(m_content);
    bodyLayout->addStretch(1);

    auto *scroll = new QScrollArea(this);
    scroll->setWidget(body);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 8, 12);
    layout->addLayout(header);
    layout->addWidget(scroll, 1);

    connect(closeButton, &QPushButton::clicked, this, &AssetInfoPanel::closeRequested);
    connect(m_client, &ImmichClient::assetDetailsLoaded, this,
            [this](const ImmichAssetDetails &details) {
                if (details.asset.id == m_assetId)
                    showDetails(details);
            });
    connect(m_client, &ImmichClient::assetDetailsFailed, this,
            [this](const QString &assetId, const QString &message) {
                if (assetId == m_assetId)
                    m_status->setText(tr("Details unavailable\n%1").arg(message));
            });
    connect(m_client, &ImmichClient::assetAlbumsLoaded, this,
            [this](const QString &assetId, const QList<ImmichAlbum> &albums) {
                m_albumsByAsset.insert(assetId, albums);
                if (assetId == m_assetId)
                    showAlbums(albums);
            });
    connect(m_client, &ImmichClient::assetAlbumsFailed, this,
            [this](const QString &assetId, bool permissionDenied) {
                const QString message = permissionDenied
                                            ? tr("Albums need the album.read permission")
                                            : tr("Albums unavailable");
                m_albumErrors.insert(assetId, message);
                if (assetId == m_assetId && m_albumsLabel)
                    m_albumsLabel->setText(message);
            });
    connect(m_client, &ImmichClient::personThumbnailLoaded, this,
            [this](const QString &personId, const QPixmap &thumbnail) {
                if (QLabel *avatar = m_avatars.value(personId)) {
                    const qreal ratio = devicePixelRatioF();
                    QPixmap pixmap =
                        circularPixmap(thumbnail, QSize(kAvatarSize, kAvatarSize) * ratio);
                    pixmap.setDevicePixelRatio(ratio);
                    avatar->setText({});
                    avatar->setPixmap(pixmap);
                }
            });
}

void AssetInfoPanel::setAsset(const ImmichAsset &asset)
{
    m_assetId = asset.id;
    if (isVisible())
        requestDetails();
}

void AssetInfoPanel::showEvent(QShowEvent *event)
{
    QFrame::showEvent(event);
    if (m_requestedAssetId != m_assetId)
        requestDetails();
}

void AssetInfoPanel::requestDetails()
{
    m_requestedAssetId = m_assetId;
    m_albumsByAsset.remove(m_assetId);
    m_albumErrors.remove(m_assetId);
    clearContent();
    m_status->setText(tr("Loading details…"));
    m_status->show();
    m_client->fetchAssetDetails(m_assetId);
}

void AssetInfoPanel::clearContent()
{
    while (QLayoutItem *item = m_content->takeAt(0)) {
        if (QWidget *widget = item->widget())
            widget->deleteLater();
        delete item;
    }
    m_albumsBox = nullptr;
    m_albumsLabel = nullptr;
    m_avatars.clear();
}

void AssetInfoPanel::showDetails(const ImmichAssetDetails &details)
{
    clearContent();
    m_status->hide();
    const QLocale locale;

    if (details.localDateTime.isValid()) {
        const QDate date = details.localDateTime.date();
        auto *dateLink = makeLink(locale.toString(date, QLocale::LongFormat));
        dateLink->setToolTip(tr("Show photos from this day"));
        connect(dateLink, &QPushButton::clicked, this, [this, date] { emit dateRequested(date); });
        QString time = locale.toString(details.localDateTime.time(), QLocale::ShortFormat);
        if (!details.timeZone.isEmpty())
            time += QStringLiteral(" (%1)").arg(details.timeZone);
        addSection(QStringLiteral(":/icons/calendar.svg"), tr("Date taken"),
                   {dateLink, makeLabel(time, true)});
    }

    QList<QWidget *> fileLines;
    if (!details.asset.fileName.isEmpty())
        fileLines << makeLabel(details.asset.fileName);
    QStringList fileFacts;
    if (details.width > 0 && details.height > 0) {
        fileFacts << QStringLiteral("%1 × %2").arg(details.width).arg(details.height);
        fileFacts << tr("%1 MP").arg(
            locale.toString(details.width * qint64(details.height) / 1e6, 'f', 1));
    }
    if (details.fileSize > 0)
        fileFacts << locale.formattedDataSize(details.fileSize);
    if (!fileFacts.isEmpty())
        fileLines << makeLabel(fileFacts.join(QStringLiteral(" · ")), true);
    if (!fileLines.isEmpty())
        addSection(QStringLiteral(":/icons/image.svg"), tr("File"), fileLines);

    QList<QWidget *> cameraLines;
    if (!details.make.isEmpty() || !details.model.isEmpty())
        cameraLines << makeLabel(cameraName(details));
    if (!details.lens.isEmpty())
        cameraLines << makeLabel(details.lens, true);
    if (const QString exposure = exposureSummary(details); !exposure.isEmpty())
        cameraLines << makeLabel(exposure, true);
    if (!cameraLines.isEmpty())
        addSection(QStringLiteral(":/icons/camera.svg"), tr("Camera"), cameraLines);

    if (details.hasLocation || !details.city.isEmpty()) {
        QList<QWidget *> locationLines;
        QStringList place;
        for (const QString &part : {details.city, details.state, details.country}) {
            if (!part.isEmpty() && !place.contains(part))
                place << part;
        }
        if (!details.city.isEmpty()) {
            auto *placeLink = makeLink(place.join(QStringLiteral(", ")));
            placeLink->setToolTip(tr("Show photos from %1").arg(details.city));
            connect(placeLink, &QPushButton::clicked, this,
                    [this, city = details.city] { emit placeRequested(city); });
            locationLines << placeLink;
        } else if (!place.isEmpty()) {
            locationLines << makeLabel(place.join(QStringLiteral(", ")));
        }
        if (details.hasLocation) {
            locationLines << makeLabel(QStringLiteral("%1, %2")
                                           .arg(locale.toString(details.latitude, 'f', 5),
                                                locale.toString(details.longitude, 'f', 5)),
                                       true);
            auto *map = makeLabel(tr("Map coming soon"), true);
            map->setObjectName(QStringLiteral("mapPlaceholder"));
            map->setAlignment(Qt::AlignCenter);
            map->setMinimumHeight(96);
            locationLines << map;
        }
        addSection(QStringLiteral(":/icons/map-pin.svg"), tr("Location"), locationLines);
    }

    if (details.sharedByOtherUser) {
        addSection(QStringLiteral(":/icons/share.svg"), tr("Shared by"),
                   {makeLabel(tr("Shared by %1").arg(details.ownerName))});
    }

    if (details.people.isEmpty()) {
        addSection(QStringLiteral(":/icons/users.svg"), tr("People"),
                   {makeLabel(tr("No people recognized"), true)});
    } else {
        auto *grid = new QWidget;
        auto *gridLayout = new QGridLayout(grid);
        gridLayout->setContentsMargins(0, 4, 0, 0);
        gridLayout->setHorizontalSpacing(8);
        gridLayout->setVerticalSpacing(10);
        for (int i = 0; i < details.people.size(); ++i) {
            const ImmichPerson &person = details.people.at(i);
            auto *button = new QPushButton(grid);
            button->setProperty("personButton", true);
            button->setCursor(Qt::PointingHandCursor);
            button->setToolTip(tr("Show photos of %1")
                                   .arg(person.name.isEmpty() ? tr("this person") : person.name));
            auto *buttonLayout = new QVBoxLayout(button);
            buttonLayout->setContentsMargins(2, 2, 2, 2);
            buttonLayout->setSpacing(4);
            auto *avatar = new QLabel(button);
            avatar->setObjectName(QStringLiteral("personAvatar"));
            avatar->setFixedSize(kAvatarSize, kAvatarSize);
            avatar->setAlignment(Qt::AlignCenter);
            avatar->setText(person.name.left(1).toUpper());
            const QString fullName = person.name.isEmpty() ? tr("Unnamed") : person.name;
            auto *name = new QLabel(button);
            name->setAlignment(Qt::AlignHCenter);
            name->setText(name->fontMetrics().elidedText(fullName, Qt::ElideRight, 72));
            name->setProperty("subheading", true);
            for (QLabel *label : {avatar, name})
                label->setAttribute(Qt::WA_TransparentForMouseEvents);
            buttonLayout->addWidget(avatar, 0, Qt::AlignHCenter);
            buttonLayout->addWidget(name);
            button->setFixedSize(76, kAvatarSize + 32);
            gridLayout->addWidget(button, i / 3, i % 3, Qt::AlignLeft);
            connect(button, &QPushButton::clicked, this,
                    [this, person] { emit personRequested(person); });
            m_avatars.insert(person.id, avatar);
            m_client->loadPersonThumbnail(person.id);
        }
        addSection(QStringLiteral(":/icons/users.svg"), tr("People"), {grid});
    }

    addSection(QStringLiteral(":/icons/info.svg"), tr("Description"),
               {details.description.isEmpty() ? makeLabel(tr("No description"), true)
                                              : makeLabel(details.description)});

    // Albums arrive from a separate request; reserve their place in the order.
    m_albumsLabel = makeLabel(tr("Loading albums…"), true);
    m_albumsBox = new QWidget;
    auto *albumsLayout = new QVBoxLayout(m_albumsBox);
    albumsLayout->setContentsMargins(0, 0, 0, 0);
    albumsLayout->setSpacing(2);
    albumsLayout->addWidget(m_albumsLabel);
    addSection(QStringLiteral(":/icons/book-image.svg"), tr("Albums"), {m_albumsBox});
    if (m_albumsByAsset.contains(details.asset.id))
        showAlbums(m_albumsByAsset.value(details.asset.id));
    else if (m_albumErrors.contains(details.asset.id))
        m_albumsLabel->setText(m_albumErrors.value(details.asset.id));

    QList<QWidget *> tagLines;
    for (const ImmichTag &tag : details.tags) {
        auto *link = makeLink(elided(tag.path));
        link->setToolTip(tr("Show photos tagged %1").arg(tag.path));
        connect(link, &QPushButton::clicked, this, [this, tag] { emit tagRequested(tag); });
        tagLines << link;
    }
    if (tagLines.isEmpty())
        tagLines << makeLabel(tr("No tags"), true);
    addSection(QStringLiteral(":/icons/tag.svg"), tr("Tags"), tagLines);

    if (!details.folder.isEmpty()) {
        // Keep the start and the end of a long path; the tooltip has all of it.
        // A label rather than a link button so long paths wrap.
        auto *folder = makeLabel(details.folder);
        folder->setProperty("linkLabel", true);
        folder->setTextInteractionFlags(Qt::NoTextInteraction);
        folder->setCursor(Qt::PointingHandCursor);
        folder->setToolTip(tr("Show the photos in this folder"));
        folder->setProperty("folderPath", details.folderPath);
        folder->installEventFilter(this);
        addSection(QStringLiteral(":/icons/folder.svg"), tr("Folder"), {folder});
    }
}

bool AssetInfoPanel::eventFilter(QObject *watched, QEvent *event)
{
    auto *label = qobject_cast<QLabel *>(watched);
    if (label && label->property("folderPath").isValid()) {
        if (event->type() == QEvent::Enter || event->type() == QEvent::Leave) {
            QFont font = label->font();
            font.setUnderline(event->type() == QEvent::Enter);
            label->setFont(font);
        } else if (event->type() == QEvent::MouseButtonRelease &&
                   static_cast<QMouseEvent *>(event)->button() == Qt::LeftButton &&
                   label->rect().contains(static_cast<QMouseEvent *>(event)->position().toPoint())) {
            emit folderRequested(label->property("folderPath").toString(),
                                 label->text().section(u'/', -1));
            return true;
        }
    }
    return QFrame::eventFilter(watched, event);
}

void AssetInfoPanel::showAlbums(const QList<ImmichAlbum> &albums)
{
    if (!m_albumsBox)
        return;
    if (albums.isEmpty()) {
        m_albumsLabel->setText(tr("Not in any album"));
        return;
    }
    m_albumsLabel->hide();
    for (const ImmichAlbum &album : albums) {
        auto *link = makeLink(elided(album.name));
        link->setToolTip(tr("Show the album %1").arg(album.name));
        connect(link, &QPushButton::clicked, this, [this, album] { emit albumRequested(album); });
        m_albumsBox->layout()->addWidget(link);
    }
}

QWidget *AssetInfoPanel::addSection(const QString &iconResource, const QString &title,
                                    const QList<QWidget *> &lines)
{
    auto *section = new QWidget;
    auto *row = new QHBoxLayout(section);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(12);

    auto *icon = new QLabel(section);
    icon->setFixedSize(18, 18);
    icon->setToolTip(title);
    icon->setAccessibleName(title);
    icon->setPixmap(renderSvgIcon(iconResource, palette().color(QPalette::PlaceholderText),
                                  QSize(18, 18), devicePixelRatioF()));
    row->addWidget(icon, 0, Qt::AlignTop);

    auto *column = new QVBoxLayout;
    column->setObjectName(QStringLiteral("sectionLines"));
    column->setSpacing(2);
    for (QWidget *line : lines)
        column->addWidget(line);
    row->addLayout(column, 1);

    m_content->addWidget(section);
    return section;
}

QLabel *AssetInfoPanel::makeLabel(const QString &text, bool muted)
{
    auto *label = new QLabel(text);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    if (muted)
        label->setProperty("subheading", true);
    return label;
}

QString AssetInfoPanel::elided(const QString &text, Qt::TextElideMode mode) const
{
    return fontMetrics().elidedText(text, mode, kLinkWidth);
}

QPushButton *AssetInfoPanel::makeLink(const QString &text)
{
    auto *link = new QPushButton(text);
    link->setProperty("linkButton", true);
    link->setCursor(Qt::PointingHandCursor);
    return link;
}

} // namespace Aurora
