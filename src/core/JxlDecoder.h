#pragma once

#include <QByteArray>
#include <QImage>

namespace Aurora {

// True when this build was linked with libjxl.
bool jxlDecodingAvailable();
// True when the bytes look like a JPEG XL codestream or ISO BMFF container.
bool looksLikeJxl(const QByteArray &bytes);
// Decodes a JXL still image. Returns a null image when unsupported or on error.
QImage decodeJxl(const QByteArray &bytes, int maximumDimension);

} // namespace Aurora
