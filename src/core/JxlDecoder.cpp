#include "core/JxlDecoder.h"

#include <QSize>
#include <cstring>
#include <vector>

#if defined(IMMICH_HAS_LIBJXL)
#include <jxl/decode.h>
#include <jxl/decode_cxx.h>
#include <jxl/resizable_parallel_runner_cxx.h>
#endif

namespace Aurora {

bool looksLikeJxl(const QByteArray &bytes)
{
    if (bytes.size() < 2)
        return false;
    const auto *data = reinterpret_cast<const unsigned char *>(bytes.constData());
    // Raw codestream.
    if (data[0] == 0xff && data[1] == 0x0a)
        return true;
    // ISO BMFF container: ....JXL \r\n\x87\n
    if (bytes.size() >= 12 && std::memcmp(data + 4, "JXL \r\n\x87\n", 8) == 0)
        return true;
    return false;
}

bool jxlDecodingAvailable()
{
#if defined(IMMICH_HAS_LIBJXL)
    return true;
#else
    return false;
#endif
}

QImage decodeJxl(const QByteArray &bytes, int maximumDimension)
{
#if !defined(IMMICH_HAS_LIBJXL)
    Q_UNUSED(bytes);
    Q_UNUSED(maximumDimension);
    return {};
#else
    if (bytes.isEmpty() || !looksLikeJxl(bytes))
        return {};

    auto runner = JxlResizableParallelRunnerMake(nullptr);
    auto decoder = JxlDecoderMake(nullptr);
    if (!decoder || !runner)
        return {};

    if (JxlDecoderSubscribeEvents(decoder.get(),
                                  JXL_DEC_BASIC_INFO | JXL_DEC_FULL_IMAGE) != JXL_DEC_SUCCESS)
        return {};
    if (JxlDecoderSetParallelRunner(decoder.get(), JxlResizableParallelRunner,
                                    runner.get()) != JXL_DEC_SUCCESS)
        return {};
    if (JxlDecoderSetInput(decoder.get(),
                           reinterpret_cast<const uint8_t *>(bytes.constData()),
                           size_t(bytes.size())) != JXL_DEC_SUCCESS)
        return {};
    JxlDecoderCloseInput(decoder.get());

    JxlBasicInfo info = {};
    std::vector<uint8_t> pixels;
    QSize imageSize;
    for (;;) {
        const JxlDecoderStatus status = JxlDecoderProcessInput(decoder.get());
        if (status == JXL_DEC_ERROR || status == JXL_DEC_NEED_MORE_INPUT)
            return {};
        if (status == JXL_DEC_SUCCESS)
            break;
        if (status == JXL_DEC_BASIC_INFO) {
            if (JxlDecoderGetBasicInfo(decoder.get(), &info) != JXL_DEC_SUCCESS)
                return {};
            if (info.xsize == 0 || info.ysize == 0)
                return {};
            imageSize = QSize(int(info.xsize), int(info.ysize));
            if (maximumDimension > 0 &&
                (imageSize.width() > maximumDimension || imageSize.height() > maximumDimension)) {
                imageSize.scale(maximumDimension, maximumDimension, Qt::KeepAspectRatio);
            }
            JxlResizableParallelRunnerSetThreads(
                runner.get(),
                JxlResizableParallelRunnerSuggestThreads(info.xsize, info.ysize));
            continue;
        }
        if (status == JXL_DEC_NEED_IMAGE_OUT_BUFFER) {
            JxlPixelFormat format = {4, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0};
            // Decode at full size into a buffer, then scale with Qt when needed.
            format.num_channels = 4;
            size_t bufferSize = 0;
            if (JxlDecoderImageOutBufferSize(decoder.get(), &format, &bufferSize) !=
                JXL_DEC_SUCCESS)
                return {};
            pixels.resize(bufferSize);
            if (JxlDecoderSetImageOutBuffer(decoder.get(), &format, pixels.data(),
                                            pixels.size()) != JXL_DEC_SUCCESS)
                return {};
            continue;
        }
        if (status == JXL_DEC_FULL_IMAGE)
            continue;
        return {};
    }

    if (pixels.empty() || info.xsize == 0 || info.ysize == 0)
        return {};

    QImage image(int(info.xsize), int(info.ysize), QImage::Format_RGBA8888);
    if (image.isNull())
        return {};
    const qsizetype rowBytes = qsizetype(info.xsize) * 4;
    for (uint32_t y = 0; y < info.ysize; ++y) {
        std::memcpy(image.scanLine(int(y)), pixels.data() + qsizetype(y) * rowBytes,
                    size_t(rowBytes));
    }
    image = image.convertToFormat(QImage::Format_ARGB32);
    if (maximumDimension > 0 &&
        (image.width() > maximumDimension || image.height() > maximumDimension)) {
        image = image.scaled(maximumDimension, maximumDimension, Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
    }
    return image;
#endif
}

} // namespace Aurora
