#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMovie>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QMediaPlaylist>
#include <QtMultimediaWidgets/QVideoWidget>
//#include <QtMultimedia/QMetaDataReaderControl>
#include <QUrl>
#include <QFileInfo>
#include <QProcess>
#include <QDropEvent>
#include <QLabel>
#include <QDockWidget>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:

    void on_toolButtonFileDialog_clicked();
    void on_toolButtonPlay_clicked();
    void on_toolButtonPause_clicked();
    void on_toolButtonStop_clicked();

    void mediaStateChanged(QMediaPlayer::State state);
    void durationChanged(qint64 duration);
    void setPosition(qint64 position);
    void setPosition2(int position);
    void positionChanged(qint64 position);
    void metaDataAvailableChanged(bool);
    void metaDataChanged();
    void volumeChanged(int volume);

    void on_horizontalSliderVideo_valueChanged(int value);

    void on_horizontalSliderVolume_sliderMoved(int position);

    void on_toolButtonBeginSet_clicked();

    void on_toolButtonEndSet_clicked();

    void on_timeEditBegin_timeChanged(const QTime &time);

    void on_timeEditEnd_userTimeChanged(const QTime &time);

    void on_toolButtonClipClear_clicked(bool checked);

    void on_toolButtonClipAdd_clicked();

    void on_toolButtonTagAll_clicked();

    void on_pushButtonReset_clicked();

    void on_pushButtonVLGen_clicked();

    void customContextMenuRequested(QPoint pos);

    void on_listWidgetTags_itemSelectionChanged();

    void on_toolButton_clicked();

    void on_toolButtonBegin_clicked();

    void on_toolButtonEnd_clicked();

    void ffprobeVideoOutput();

    void on_pushButtonUpdate_clicked();

    void on_horizontalSliderVolume_valueChanged(int value);

    void on_dockWidget_dockLocationChanged(const Qt::DockWidgetArea &area);

    void on_dockWidget_featuresChanged(const QDockWidget::DockWidgetFeatures &features);

    void on_dockWidget_topLevelChanged(bool topLevel);

protected:
    void dropEvent(QDropEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void keyPressEvent(QKeyEvent *event);

private:
    Ui::MainWindow *ui;

    QMediaPlayer *mediaPlayer;
    QVideoWidget *videoWidget;
    QFileInfo *fileInfo;
    QProcess  *ffprobeProcess;
    bool clipGenerated;
    bool ffprobeMissing;

    QLabel *videoStatusLbl;
    QLabel *audioStatusLbl;
    QLabel *videoStatus;
    QLabel *audioStatus;

    void CalulateClipLength();
    void CheckExistClipFile();
    void LoadTagListIni();
    void ffprobeVideo();
    void InitializeMediaPlayer();
    bool RenameVideoFile();
    void CheckSelectedFile(QString s);
    bool GenerateClipFile(QString &clipFileName);
};

#endif // MAINWINDOW_H
