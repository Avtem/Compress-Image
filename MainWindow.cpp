#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QFileDialog>
#include <QBitmap>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDebug>
#include "DynamicInt8.h"

#define tostr(numb) QString::number(numb)
#define tochars(numb) QString::number(numb).toStdString().c_str()
#define tostrHex(numb) QString::number(numb, 16)
#define tostrF(numb) QString::number(numb, 'f', 2)
template <typename passedType>
QString toStrWCommas(passedType number)
{
    QLocale aEnglish(QLocale(QLocale::English, QLocale::UnitedStates));
    return aEnglish.toString(static_cast<double>(number), 'f', 0);
}

const QString AvimAttributes::avtemMark = "AVTEM IMAGE FILE $*(@#*$#@)";



void MainWindow::putImgInLabel(const QImage &img)
{
    if(img.isNull())
        return;

    mImg = img.convertToFormat(QImage::Format_RGB888);

    QPixmap pixmap = QPixmap::fromImage(mImg);
    pixmap = pixmap.scaled(ui->labelimg->width(), ui->labelimg->height(),
                           Qt::AspectRatioMode::KeepAspectRatio);
    ui->labelimg->setPixmap(pixmap);
    ui->labelimg->setMask(pixmap.mask());
}

void MainWindow::refreshAllLabels(bool their)
{
    ui->labelWidthHeight->setText(tostr(mImg.width())
                                    +'x' + tostr(mImg.height()));
    ui->labelPixelCount->setText(toStrWCommas(pixelCount()) + " pixels");
    ui->labelNonComprSize->setText(toStrWCommas(pixelCount() *3) + " bytes");

    double sizeOnDisk = QFileInfo(mSrcImagePath).size();
    qDebug() << "source path: " << mSrcImagePath;

    if(their) {
        ui->labelSizeOnDiskTheir->setText(toStrWCommas(sizeOnDisk) + " bytes");
        ui->labelCompMultiplierTheir->setText("CF = "
                                       + tostrF(pixelCount()*3/sizeOnDisk) +'x');
    }
    else {
        ui->labelSizeOnDiskAvtem->setText(toStrWCommas(sizeOnDisk) + " bytes");
        ui->labelCompMultiplierAvtem->setText("CF = "
                                       + tostrF(pixelCount()*3/sizeOnDisk) +'x');
    }
}

void MainWindow::refreshAvtemLabels(QString pathToAvimFile)
{
    QFileInfo fileInfo(pathToAvimFile);
    double sizeOnDisk = fileInfo.size();
    ui->labelSizeOnDiskAvtem->setText(toStrWCommas(sizeOnDisk) + " bytes");

    if(sizeOnDisk != 0.0)
      ui->labelCompMultiplierAvtem->setText("CF = "
       + tostrF(static_cast<double>(pixelCount()*3) /sizeOnDisk) + 'x');
}

int MainWindow::countColors()
{
    mFoundColors.clear();

    const uchar *imgPixels = mImg.constBits();
//    QMap <QRgb, uint> colorMap;  // debug only
//    uint colorMaps = 0;         // debug only

    for(int pixel=0; pixel < pixelCount(); pixel++)
    {
        uint key = qRgb(*(imgPixels), *(imgPixels +1), *(imgPixels +2));
//                if(!mFoundColors[key]) { // color hasn't been added
//                    if(colorMap.count() %256 == 0) {
//                        colorMaps ++;
//                        qDebug() << "New color tableRequired! on Y: " << y;
//                    }
//                    colorMap[key] = 7;
//                }
        mFoundColors[key] = 0;
        imgPixels += 3;
    }

//    qDebug() << endl << "ColorMaps created: " << colorMaps << "size: " << colorMaps *256;
    return mFoundColors.size();
}

void MainWindow::makeColorMaps()
{
    mColorMaps.clear();

    const uchar *imgPixels = mImg.constBits();
    do {
        mColorMaps.append(QVector<uint>()); // add another color map
        const QVector<uint> &thisColorMap = mColorMaps.constLast();

        mColorMaps.last().push_back(0);    // push back reserved items
        mColorMaps.last().push_back(256);  // CHANGE IT AFTER COLMAP FILLED!

        switch (mReservedItemCount) {
            case 3: mColorMaps.last().push_back(1); // repeatCountItem
        }
        QRgb currPixel = getRGBuint(imgPixels);


        while(imgPixels != lastPixelPtr() +3 // end of the image OR filled
              && mColorMaps.constLast().count() != 256)
        {
            if(mColorMaps.constLast().indexOf(currPixel) < mReservedItemCount)
                mColorMaps.last().push_back(currPixel);  // push new color

            imgPixels +=3;
            currPixel = getRGBuint(imgPixels);
        }
        mColorMaps.last()[1] = uint(mColorMaps.constLast().count()
                                    -mReservedItemCount);
    } while(imgPixels != lastPixelPtr() +3); // end of the image!!!

//    for(int i=0; i < mColorMaps.count(); i++)
//        std::sort(mColorMaps[i].begin(), mColorMaps[i].end());

//        for(int j=mReservedItemCount; j < mColorMaps[i].count(); j++)
//            mColorMaps[i][j] -= 0xFF000000;
//    }

    for(int i=0; i < mColorMaps.count(); i++)
    {
        int plusEqualLess127 =0, plusEqualLess16k =0, plusEqualLess2mln =0; // debug
        const QVector<QRgb> con = mColorMaps.at(i); // debug

        for(int j=mReservedItemCount+1; j < mColorMaps.at(i).count(); j++) {
            uint first = mColorMaps.at(i).at(j);
            uint second = mColorMaps.at(i).at(j -1);
            uint incrementBy = first - second;
            if(incrementBy <= 127)
                plusEqualLess127 ++;
            else if(incrementBy <= 16383)
                plusEqualLess16k ++;
            else if(incrementBy <= 2097151)
                plusEqualLess2mln ++;
//            else
//                qDebug() << "oh my god, increment is so large!!!";
        }
//        qDebug() << "     clor map #" << i +1 ;
//        qDebug() << "up to 127: " << plusEqualLess127;
//        qDebug() << "up to 16k: " << plusEqualLess16k;
//        qDebug() << "up to 2ml: " << plusEqualLess2mln;
    }

    qDebug() << "color maps created: " << mColorMaps.count();
}

int MainWindow::pixelCount() const
{
    return mImg.width() * mImg.height();
}

const uchar *MainWindow::lastPixelPtr() const
{
    return mImg.constBits() + pixelCount()*3 -3;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mAvimAttributes(nullptr)
{
    mReservedItemCount = 2;   // how many bytes color maps use for "stuff"?

    ui->setupUi(this);
    ui->labelCompMultiplierTheir->setStyleSheet("QLabel { background-color : dimGray; color : orange; }");
    ui->labelCompMultiplierAvtem->setStyleSheet("QLabel { background-color : dimGray; color : lime; }");


    connect(ui->btnOpenTheir, &QPushButton::clicked,
            this, &MainWindow::openTheirFile);

    if(ui->cmboxAvtemCompr->count())
        ui->cmboxAvtemCompr->setCurrentIndex(ui->cmboxAvtemCompr->count() -1);
}

MainWindow::~MainWindow()
{
    if(mAvimAttributes)
        delete mAvimAttributes;

    delete ui;
}

void MainWindow::openTheirFile()
{
    resetLabelsWithPercentage();
    mSrcImagePath = QFileDialog::getOpenFileName(this, "Select an image",
        "T:/0101_Programming/activeProjects/Qt_projects/CompressImage/"
        "build-CompressImage-Desktop_Qt_5_12_1_MinGW_64_bit-Debug/testImgs");

    putImgInLabel(QImage(mSrcImagePath));
    refreshAllLabels(true);
    int colorCount = countColors();
    ui->labelColorCount->setText(tostr(colorCount)); // + count colors
    ui->labelColRR->setText(tostrF(static_cast<double>(pixelCount())
                                   / colorCount));
    ui->labelCompMultiplierAvtem->setText("?");
    ui->labelSizeOnDiskAvtem->setText("?");
}

void MainWindow::loadAsAVIM2()
{  //AVIM2   // colMap[256]colMap[52]idx[1]idx[1]idx[1][0]idx[1]// ONE MORE COLOR MAP
    mColorMaps.clear();
    QByteArray &data = mAvimAttributes->bytesData;

    int pos=0; // position in bytesData
    while(pos < data.size())      // read all color maps from the img
    {
        if(data.at(pos) != 0)   // 0 item is not a NULL. It's not a colorMap!
            break;
        pos++; // skip 0th item is NULL
        mColorMaps.append(QVector<QRgb>()); // add color map

        int currColorMapSize = quint8(data.at(pos)) +2; // 1st item is colorCount
        mColorMaps.last().reserve(currColorMapSize); // reserve size for all colors
        mColorMaps.last().push_back(0); // push 0
        mColorMaps.last().push_back(uint(currColorMapSize)); // push colorCount
        pos ++; // skip 1st item(colorCount)

        while(mColorMaps.constLast().count() != currColorMapSize)
        {    // read all colors
            mColorMaps.last().push_back(qRgb(quint8(data.at(pos)),
                              quint8(data.at(pos+1)), quint8(data.at(pos+2))));
            pos += 3;
        }
    }

    int x=0, y=0;               // reading pixels from colorMap items
    int width = mAvimAttributes->imgSize.width();
    int height = mAvimAttributes->imgSize.height();
    int colorMapIdx=0;
    while(pos < data.count())
    {
        while(data.at(pos) != 0)
        {
            mImg.setPixel(x,y, mColorMaps.at(colorMapIdx).at(uchar(data.at(pos))));
            QRgb color = mColorMaps.at(colorMapIdx).at(uchar(data.at(pos)));

            if(x+1 == width)    // iterate x,y pos
                y++;
            x = (x +1)%width;

            pos++;

            if(y == height) // end of the image
                return;
        }
        pos ++; // sKip 1st byte
        colorMapIdx ++;
    }
}

void MainWindow::loadAsAVIM3()
{
    QByteArray &data = mAvimAttributes->bytesData;

    mColorMaps.clear();

    int pos=0; // position in bytesData
    while(data.at(pos) == 0)      // read all color maps from the img
    {
        pos++; // skip 0th item is NULL
        mColorMaps.append(QVector<QRgb>()); // add color map

        // 1st item is colorCount
        int colorCount = quint8(data.at(pos));
        mColorMaps.last().reserve(colorCount +mReservedItemCount); // reserve size for all colors
        pos ++; // skip 1st item(colorCount)

        mColorMaps.last().push_back(0); // push 0
        mColorMaps.last().push_back(uint(colorCount)); // push colorCount
        for(int j=0; j < mReservedItemCount -2; j++)  // fill all reserved items
            mColorMaps.last().push_back(0);      // that weren't written to the file


        while(mColorMaps.constLast().count() != colorCount +mReservedItemCount)
        {    // read all colors
            mColorMaps.last().push_back(qRgb(quint8(data.at(pos)),
                              quint8(data.at(pos+1)), quint8(data.at(pos+2))));
            pos += 3;
        }
    }

    qDebug() << "Loaded Color map count: " << mColorMaps.count();

    int x=0, y=0;               // reading pixels from colorMap items
    int width = mAvimAttributes->imgSize.width();
    int height = mAvimAttributes->imgSize.height();
    int currColorMap=0;
    QRgb currPixel;
    DynamicInt8 repeatCount(0);

    while(pos < data.count())  //  read every byte
    {
        while(data.at(pos) != 0)    // read everything in current colMap chunk
        {
            currPixel = mColorMaps.at(currColorMap).at(uchar(data.at(pos)));
            const QVector<uint> &currColorMab = mColorMaps.at(currColorMap);
            pos ++; // go to NEXT byte

            if(uchar(data.at(pos)) == 1) {  // we have repeat count!
                pos ++; // skip item '1'

                repeatCount.clear();
                while(uchar(data.at(pos)) >> 7) {
                    repeatCount.pushBack(uchar(data.at(pos)));
                    pos ++;  // go to next byte
                }
                repeatCount.pushBack(uchar(data.at(pos))); // push byte with MSB == 0

                qDebug() << "repeat count is: " << repeatCount.asUint();
                for(uint j=0; j < repeatCount.asUint(); j++) { // set currPixel
                    mImg.setPixel(x, y, currPixel);          // repeatCount times

                    if(x+1 == width) y++;   // SET Y
                    x = (x +1)%width;       // SET X

                    if(y == height) // end of the image
                        return;
                }

                pos++; // go to NEXT pixel
            }
            else  {     // SET SINGLE PIXEL
                mImg.setPixel(x, y, currPixel);     // this pxel hasn't repeatCount=(
                if(x+1 == width) y++;   // SET Y
                x = (x +1)%width;       // SET X

                if(y == height) // end of the image
                    return;

                if(data.at(pos) == 0)  // go to next colorMap /end loading
                    break;
            }
        }
        pos ++; // sKip 0 item
        currColorMap ++;
    }
}

MainWindow::CompressChoice MainWindow::currChoice() const
{
    return static_cast<CompressChoice>(ui->cmboxAvtemCompr->currentIndex());
}

QString MainWindow::currSavePath() const
{
    QFileInfo fileInfo = QFileInfo(mSrcImagePath);  // isitsafe?
    QString suffix = currChoice() >= AVIM1 ? ".avim" : ".aimg";
    int generation = currChoice() >= AVIM1 ? (currChoice() - AVIM1+1)
                                           : currChoice();

    return "savedAIMGS/" + fileInfo.fileName().remove('.'+fileInfo.suffix())
                                            + suffix + tostr(generation);
}

void MainWindow::saveAVIMheader(const QString &filePath)
{
    if(mImg.isNull() || filePath.isEmpty())
        return;

    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    QTextStream out(&file);

    out << AvimAttributes::avtemMark;  // SETTINGS   /1
    out << "\ncompressiong algorithm: " << ui->cmboxAvtemCompr->currentIndex();//2
    out << "\nColor sequence: R,G,B";   //3rd line
    out << "\nSize: " << mImg.width() << 'x' << mImg.height(); // 4
    out << "\nReserved Count: " << mReservedItemCount;  // 5
    for(int i=0; i < 4; i++) // reserved  // 6,7,8,
        out << endl;
    out << static_cast<char>(3) << static_cast<char>(3) << static_cast<char>(3)
        << endl;    // 9 (so colormaps start at 10th line in file)

    file.close();
}

void MainWindow::saveColorMapsToFileAvim2(const QString &filePath)
{
    QFile file(filePath);
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QDataStream out(&file);

    makeColorMaps();

    uint colorMapsTookBytes = 0;
    for(int i=0; i < mColorMaps.count(); i++) {
        out << uchar(mColorMaps.at(i).at(0)); // NULL (beg.of the map)
        out << uchar(mColorMaps.at(i).at(1)); // colorCountInMap
        colorMapsTookBytes += 2; // for 0 and mapColorCount
        colorMapsTookBytes += uchar(mColorMaps.at(i).at(1)) *3;
        qDebug() << "colorMapTookBytes: " << colorMapsTookBytes;
//        switch (mReservedItemCount) {
//        case 3: out << uchar(mColorMaps.at(i).at(1)); // 1 (symbol for rep.count)
        // DO NOT SAVE OTHER RESERVED ITEMS!
//        }

        uchar mapColorRgb[3];
        for(int j=mReservedItemCount; j < mColorMaps.at(i).count(); j++)
        {
            getRGB(mColorMaps.at(i).at(j), mapColorRgb);
            out << uchar(mapColorRgb[0]) << uchar(mapColorRgb[1]) <<
                                               uchar(mapColorRgb[2]);
        } // 3 bytes for each color
    }
    ui->labelColorMapsPerc->setText(asFilePercentage(colorMapsTookBytes));

    file.close();
}

void MainWindow::savePixelsAVIM2(const QString &filePath)
{
    QFile file(filePath);
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QDataStream out(&file);

    const uchar *imgPixels = mImg.constBits();
    const uchar *const lastPixelPtr = mImg.constBits() +pixelCount()*3 -3;

    uint bytesWritten = 0;
    uint x = 0;// debug

    for(int i=0; i < mColorMaps.count(); i++)
    {
        mColorMaps[i].remove(0, 2); // remove reserved items so they won't
                                    // be counted as RGB colors
        short idxInColorMap;  // from 2 ... 255

        while(true)
        {
            //// DEBUG debug
            const QVector <uint> colorMap = mColorMaps.at(i); // debug
            QRgb currPixelColor =getRGBuint(imgPixels);  // debug
            if(x == mImg.width() -1)
                qDebug() << "x: " << x << "Color: " << qRgb(*imgPixels, *(imgPixels+1)
                                                            , *(imgPixels +2));
//            qDebug() << "x: " << x%mImg.width()
//                   << "color: " << currPixelColor
//                  << "idx in colMap: "<< mColorMaps.at(i).indexOf(getRGBuint(imgPixels));


            idxInColorMap = short(mColorMaps.at(i).indexOf(getRGBuint(imgPixels)));
            if(idxInColorMap == -1 || imgPixels == lastPixelPtr +3) {// color not found in theMap!
                break;
            }

            out << uchar(idxInColorMap +mReservedItemCount);   // write color (as 1 byte)
            bytesWritten ++; x++;
            imgPixels +=3;
        }  // write pixel while it exists in currentColorMap

        out << uchar(0);  // end current map "session"
        bytesWritten ++;
    }
    ui->label1ByteIndexesPerc->setText(asFilePercentage(bytesWritten));

    file.close();
}

void MainWindow::saveAsAVIM3(const QString &filePath)
{
    Q_ASSERT(!filePath.isEmpty());
    mReservedItemCount = 3;

    saveAVIMheader(filePath);
    saveColorMapsToFileAvim2(filePath);

    savePixelsAVIM3(filePath);
}

void MainWindow::savePixelsAVIM3(const QString &filePath)
{
    QFile file(filePath);
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QDataStream out(&file);

    const uchar *imgPixels = mImg.constBits();
    short idxInColorMap = -1;
    DynamicInt8 dynInt;
    uint repeatCount;

    uint bytesRepCountWritten =0, bytes1ByteIdxWritten =0;
    for(int i=0; i < mColorMaps.count(); i++)  // proceed every pixel
    {
        do {
            const QVector <uint> &currColorMap = mColorMaps.at(i);
            idxInColorMap = short(mColorMaps.at(i).indexOf(
                                  getRGBuint(imgPixels)));

            if(idxInColorMap < mReservedItemCount) // not found in currColorMap!!!
                break;

            out << uchar(idxInColorMap);  // write this color.
            bytes1ByteIdxWritten ++;
            repeatCount = 1;
            imgPixels +=3;         // go to NEXT pixel

            while(idxInColorMap == mColorMaps.at(i).indexOf(// is NEXT index the same?
                                    getRGBuint(imgPixels) ) )
            {
                repeatCount ++;  // getting repeatCount...
                imgPixels += 3;

                if(imgPixels == lastPixelPtr() +3) // end of the image!
                    break;
            }
            if(repeatCount > 3) // write dynamicInt
            {
                out << uchar(1); // write reserved item for repeatCount

                dynInt.write(int(repeatCount));
                for(uchar j=0; j < dynInt.byteCount(); j++)
                    out << dynInt.at(j);

                bytesRepCountWritten += 1 + dynInt.byteCount();
            }
            else if(repeatCount >= 2) {
                out << uchar(idxInColorMap);   // write 2nd idx
                bytes1ByteIdxWritten ++;
                if(repeatCount == 3) {
                    out << uchar(idxInColorMap);   // write 3rd idx
                    bytes1ByteIdxWritten ++;
                }
            }

        } while(imgPixels != lastPixelPtr() +3); // end of the image!

        out << uchar(0); // end of colorMap session!
        bytes1ByteIdxWritten ++;
    }

    ui->label1ByteIndexesPerc->setText(asFilePercentage(bytes1ByteIdxWritten));
    ui->labelRepeatCountsPers->setText(asFilePercentage(bytesRepCountWritten));

    file.close();
}

void MainWindow::resetLabelsWithPercentage()
{
    ui->label1ByteIndexesPerc->setText("-");
    ui->labelRepeatCountsPers->setText("-");
    ui->labelColorMapsPerc->setText("-");
}

QString MainWindow::asFilePercentage(uint byteCount)
{
    double percentage = double(byteCount) / double(pixelCount()*3) *100;
    return toStrWCommas(byteCount) + " bytes. "
            + tostrF(percentage) + '%' ;
}

void MainWindow::saveAsAVIM2(const QString &filePath)      // 254 map colors
{
    Q_ASSERT(!filePath.isEmpty());

    mReservedItemCount = 2;  // 2 reserved "cells" in 256 color map
    saveAVIMheader(filePath);
    saveColorMapsToFileAvim2(filePath);

    savePixelsAVIM2(filePath);
}

void MainWindow::getRGB(const uint &rgbUint, uchar *rgbArray) const
{
    rgbArray[0] = rgbUint >> 16 & 0xFF; // R
    rgbArray[1] = rgbUint >> 8  & 0xFF; // G
    rgbArray[2] = rgbUint & 0xFF;       // B
}

void MainWindow::getRGB(const uchar *redPixelPtr, uchar *rgbArray) const
{
    rgbArray[0] = *redPixelPtr;
    rgbArray[1] = *(redPixelPtr +1);
    rgbArray[2] = *(redPixelPtr +2);
}

uint MainWindow::getRGBuint(const uchar *redPixelPtr) const
{
    uint result = 0xFF000000
                  + (uint(*redPixelPtr) << 16)
                  + (uint(*(redPixelPtr +1)) << 8)
                  + uint(*(redPixelPtr +2));

    return result;
}

QString MainWindow::showSaveDlg()
{
    return QFileDialog::getSaveFileName(this, "Save as 'avimX' file...");
}

void MainWindow::on_btnSaveToFile_clicked()
{
    QString path = "";
    if(mImg.isNull() || (path = showSaveDlg()).isEmpty())
        return;

    resetLabelsWithPercentage();

    switch (currChoice()) {
        case AVIM1:
            ;
            break;
        case AVIM2:
            saveAsAVIM2(path);
            break;
        case AVIM3:
            saveAsAVIM3(path);
            break;

        default:
            return;
    }

    refreshAvtemLabels(path);
}



void MainWindow::on_btnOpenAimg_clicked()
{
    resetLabelsWithPercentage();
    QString path = QFileDialog::getOpenFileName(this, "Choose 'avimX' file",
     "T:/0101_Programming/activeProjects/Qt_projects/CompressImage_gen2/"
     "build-CompressImage_gen2-Desktop_Qt_5_12_1_MinGW_64_bit-Debug/savedAIMGS");
    if(path.isEmpty())
        return;
    mSrcImagePath = path;

    QFile file(path);
    if(!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Couldn't open the file!";
        return;
    }

    mAvimAttributes = new AvimAttributes();
    QFileInfo fi(file);
    mAvimAttributes->fileSizeInBytes = quint64(fi.size());

    QTextStream in(&file);              // read attributes from the file
    QString line = file.readLine();  // 1st
    if(line.remove('\n') != AvimAttributes::avtemMark) {
        mAvimAttributes->isValid = false;
        return;
    }

    line = file.readLine();  // 2nd
    mAvimAttributes->cChoice = static_cast<CompressChoice>(
                           line.remove("compressiong algorithm: ").toInt());
    file.readLine(); // 3rd line not used
    line = file.readLine();  // 4th
    QStringList widthHeight = line.remove("Size: ").split('x');
    mAvimAttributes->imgSize = QSize(widthHeight.at(0).toInt(),
                                     widthHeight.at(1).toInt());
    line = file.readLine();  // 5th
    mReservedItemCount = uchar(line.remove("Reserved Count: ").toInt());

    for(int i=0; i < 3; i++) file.readLine();  // reserved  // 6,7,8
    file.readLine() ; // 9  etx etx etx (3 bytes for finding first byte for QDAtaStream)

    mAvimAttributes->bytesData = file.readAll();
    file.close();


    mImg = QImage(mAvimAttributes->imgSize, QImage::Format_RGB888);
    switch (mAvimAttributes->cChoice) {
        case AVIM2:
            loadAsAVIM2();
            break;
        case AVIM3:
            loadAsAVIM3();
            break;
//        case AVIM4:
//            loadAsAVIM4();
//            break;
        default:
            return;
    }

    putImgInLabel(mImg);
    refreshAllLabels(false);
    ui->labelCompMultiplierTheir->setText("?");
    ui->labelSizeOnDiskTheir->setText("?");
}


