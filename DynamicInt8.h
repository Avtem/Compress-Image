#ifndef DYNAMICINT8_H
#define DYNAMICINT8_H

#include <QObject>
                    // xb - size, 127 - max possible value (if unsigned)
class DynamicInt8   // 1b = 127; 2b = 16,383; 3b = 2,097,151; 4b = 268,435,455;
{                   // 5b = 34,359,738,367 // 6b = 4,398,046,511,104
    const int MAX_BYTE_LENGTH = 8;  // support up to 8 bytes only.

    std::vector <quint8> mBytes;
    const static quint8 False = 0;

public:
    DynamicInt8();
    DynamicInt8(const uint &number);

    bool hasNextByte(const uint &index) const;
    void write(const int &number);
    void clear();
    void pushBack(const quint8 &value);
    uint asUint() const;
    quint8 at(const uchar &byteIndex) const;
    uchar byteCount() const;
};

#endif // DYNAMICINT8_H
