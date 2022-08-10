#include "DynamicInt8.h"
#include <QtMath>

const quint8 DynamicInt8::False;

DynamicInt8::DynamicInt8()
{
    write(0);
}

DynamicInt8::DynamicInt8(const uint &number)
{
    write(number);  // todo (do we apply negative numbers?)
}

bool DynamicInt8::hasNextByte(const uint &index) const
{
    if(index >= mBytes.size())
        return false;

    return mBytes.at(index) & 0x80;     // read 1-st bit (MSB)
}

void DynamicInt8::write(const int &number)
{
    mBytes.clear();

    quint8 byteCount = 1;
    if(number != 0)
        byteCount = static_cast<quint8>(+1 + log(number)/ log(128));

    mBytes.resize(byteCount);
    for(quint8 i=0; i < byteCount; i++) {
        mBytes[i] = (number >> 7*(byteCount -i -1)) & 0xFF;
        mBytes[i] |= 0x80;  // set 1-st bit, so we know it has next byte
    }

    mBytes.back() &= 0x7F; // unset 1-st bit to mark byte as last
}

void DynamicInt8::clear()
{
    mBytes.clear();
}

void DynamicInt8::pushBack(const quint8 &value)
{
    mBytes.push_back(value);
}

uint DynamicInt8::asUint() const
{
    uint result = 0;

    for(uint i=0; i < mBytes.size(); i++) {
        result += (mBytes.at(i) & 0x7Fu) // unset 1st bits   lShift by 7
                  << 7*(mBytes.size() -i -1);  // leftShift by 7* 0,1,2,3,4 etc.
    }

    return result;
}

quint8 DynamicInt8::at(const uchar &byteIndex) const
{
    if(byteIndex >= uchar(mBytes.size()))
        return 0;

    return mBytes.at(uint(byteIndex));
}

uchar DynamicInt8::byteCount() const
{
    return uchar(mBytes.size());
}

