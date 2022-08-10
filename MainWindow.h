#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QVector>

namespace Ui {
class MainWindow;
}
struct AvimAttributes;

class MainWindow : public QMainWindow
{
    Ui::MainWindow *ui;
Q_OBJECT
    QString mSrcImagePath;
    QImage mImg;
    QMap <QRgb, uint> mFoundColors;
    QList<QVector <QRgb>> mColorMaps;   // color maps (250 each)
    uchar mReservedItemCount; // how many bytes color maps use for "stuff"?
    AvimAttributes *mAvimAttributes;

    void putImgInLabel(const QImage &img);
    void refreshAllLabels(bool their);
    void refreshAvtemLabels(QString pathToAvimFile);
    int countColors();
    void makeColorMaps();
    int pixelCount() const;
    const uchar *lastPixelPtr() const;
    QString currSavePath() const;

    void openTheirFile();

    void loadAsAVIM1();
    void loadAsAVIM2();
    void loadAsAVIM3();
    void loadAsAVIM4();

    void saveAVIMheader(const QString &filePath);
//    void saveAsAVIM1(const QString &filePath);
        // avim2
    void saveAsAVIM2(const QString &filePath);
    void saveColorMapsToFileAvim2(const QString &filePath);
    void savePixelsAVIM2(const QString &filePath);
        // avim3
    void saveAsAVIM3(const QString &filePath);
    void savePixelsAVIM3(const QString &filePath);

//    void saveAsAVIM4(const QString &filePath);

    void resetLabelsWithPercentage();
    QString asFilePercentage(uint byteCount);
    void getRGB(const uint &rgbUint, uchar *rgbArray) const;
    void getRGB(const uchar *redPixelPtr, uchar* rgbArray) const;
    uint getRGBuint(const uchar *redPixelPtr) const;
    QString showSaveDlg();
private slots:
    void on_btnSaveToFile_clicked();
    void on_btnOpenAimg_clicked();

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum CompressChoice {
        HumanReadable = 0,     // 32:122[R253 G144 B13]s
        HumanReadableSmart,    // 32:122[253,144,13]
        HumanReadableSmartHex, // 20:7A[FD,90,D]
        BinarySimple,          // •NULL#s
        BinaryLoop,            // •NULL#s•NULL# 128 3byteInt
        AVIM1,   // R[1]G[1]B[1]c[1-4b]    c = colorcount     // REPEATME
        AVIM2,   // colMap[256]rgb[1]rgb[1]rgb[1]             // ONE MORE COLOR MAP
        AVIM3    // colMaps + repeatCount
    };

    CompressChoice currChoice() const;
};

struct AvimAttributes {
    bool isValid;
    MainWindow::CompressChoice cChoice;
    QSize imgSize;
    static const QString avtemMark;
    QByteArray bytesData;

    quint64 fileSizeInBytes;
    AvimAttributes() { isValid = false; }
};

#endif // MAINWINDOW_H
