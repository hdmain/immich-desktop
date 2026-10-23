#pragma once

#include "core/ImmichClient.h"

#include <QDate>
#include <QFrame>
#include <QHash>

class QLabel;
class QPushButton;
class QShowEvent;
class QVBoxLayout;

namespace Aurora {

class AssetInfoPanel final : public QFrame {
    Q_OBJECT

public:
    explicit AssetInfoPanel(ImmichClient *client, QWidget *parent = nullptr);

    void setAsset(const ImmichAsset &asset);

signals:
    void closeRequested();
    void personRequested(const Aurora::ImmichPerson &person);
    void placeRequested(const QString &city);
    void dateRequested(const QDate &date);
    void albumRequested(const Aurora::ImmichAlbum &album);
    void tagRequested(const Aurora::ImmichTag &tag);
    void folderRequested(const QString &path, const QString &title);

protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void requestDetails();
    void showDetails(const ImmichAssetDetails &details);
    void showAlbums(const QList<ImmichAlbum> &albums);
    void clearContent();
    QWidget *addSection(const QString &iconResource, const QString &title,
                        const QList<QWidget *> &lines);
    QLabel *makeLabel(const QString &text, bool muted = false);
    QPushButton *makeLink(const QString &text);
    QString elided(const QString &text, Qt::TextElideMode mode = Qt::ElideRight) const;

    ImmichClient *m_client;
    QString m_assetId;
    QString m_requestedAssetId;
    QVBoxLayout *m_content;
    QLabel *m_status;
    QWidget *m_albumsBox = nullptr;
    QLabel *m_albumsLabel = nullptr;
    QHash<QString, QList<ImmichAlbum>> m_albumsByAsset;
    QHash<QString, QString> m_albumErrors;
    QHash<QString, QLabel *> m_avatars;
};

} // namespace Aurora
