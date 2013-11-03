#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>

#include <QFileDialog>
#include <QMessageBox>
#include <QBoxLayout>
#include <QDateTime>
#include <QMimeData>
//#include <QMediaControl>
//#include <QMediaService>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    fileInfo(NULL),
    ffprobeProcess(NULL),
    clipGenerated(false),
    ffprobeMissing(false)
{
    ui->setupUi(this);

    videoWidget = new QVideoWidget();
    ui->verticalLayout->addWidget(videoWidget);
    videoWidget->setAspectRatioMode(Qt::KeepAspectRatio);
    InitializeMediaPlayer();

    connect(ui->horizontalSliderVideo, SIGNAL(sliderMoved(int)),
            this, SLOT(setPosition2(int)));

    ui->tableWidget->setColumnWidth(4, 300);
    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableWidget, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(customContextMenuRequested(QPoint)));

    ui->lineEditVideoList->setText(QDateTime::currentDateTime().toString("yyyyMdhhmm"));

    LoadTagListIni();

    ffprobeProcess = new QProcess(ui->centralWidget);
    connect(ffprobeProcess, SIGNAL(readyRead()), this, SLOT(ffprobeVideoOutput()));

    videoStatus = new QLabel("NONE");
    audioStatus = new QLabel("UNKNOWN");
    videoStatusLbl = new QLabel("Video");
    audioStatusLbl = new QLabel("Volume");
    videoStatus->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    videoStatus->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    videoStatus->setMinimumWidth(100);
    videoStatus->setAlignment(Qt::AlignCenter);
    audioStatus->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    audioStatus->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    audioStatus->setMinimumWidth(100);
    audioStatus->setAlignment(Qt::AlignCenter);
    videoStatusLbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    videoStatusLbl->setMinimumWidth(20);
    audioStatusLbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    audioStatusLbl->setMinimumWidth(30);
    ui->statusBar->addPermanentWidget(videoStatusLbl, 0);
    ui->statusBar->addPermanentWidget(videoStatus, 0);
    ui->statusBar->addPermanentWidget(audioStatusLbl, 0);
    ui->statusBar->addPermanentWidget(audioStatus, 0);
}

MainWindow::~MainWindow()
{
    mediaPlayer->stop();
    delete mediaPlayer;

    videoWidget->close();
    delete videoWidget;

    delete ffprobeProcess;
    delete fileInfo;
    delete videoStatus;
    delete audioStatus;
    delete ui;
}

void MainWindow::InitializeMediaPlayer()
{
    mediaPlayer = new QMediaPlayer();
    mediaPlayer->setVideoOutput(videoWidget);

    connect(mediaPlayer, SIGNAL(stateChanged(QMediaPlayer::State)),
            this, SLOT(mediaStateChanged(QMediaPlayer::State)));
    connect(mediaPlayer, SIGNAL(durationChanged(qint64)),
            this, SLOT(durationChanged(qint64)));
    connect(mediaPlayer, SIGNAL(positionChanged(qint64)),
            this, SLOT(positionChanged(qint64)));
    connect(mediaPlayer, SIGNAL(metaDataAvailableChanged(bool)),
            this, SLOT(metaDataAvailableChanged(bool)));
    connect(mediaPlayer, SIGNAL(metaDataChanged()),
            this, SLOT(metaDataChanged()));
    connect(mediaPlayer, SIGNAL(volumeChanged(int)),
            this, SLOT(volumeChanged(int)));
}

void MainWindow::CheckExistClipFile()
{
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);

    if (fileInfo) {
        QString clipFileName = fileInfo->absolutePath() + "/" + fileInfo->completeBaseName() + ".clip";
        QFile qfile(clipFileName);

        if (!qfile.exists()) {
            return;
        }

        if (!qfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::information(ui->centralWidget, "Info", "\n" + clipFileName + " open failed!\n", "OK");
            return;
        }

        QTextStream in(&qfile);
        int lineCounter = 0;
        while(!in.atEnd()) {
            QString line = in.readLine();
            if (lineCounter == 0) {
                if (line != fileInfo->fileName()) {
                    break;
                }
            } else if (lineCounter == 1) {
                //ui->comboBox->setCurrentText(line);
            } else {
                QStringList list = line.split(' ');
                QStringList::Iterator it = list.begin();
                int column = 0;

                ui->tableWidget->insertRow(ui->tableWidget->rowCount());
                while(it != list.end()) {
                    ui->tableWidget->setItem(ui->tableWidget->rowCount() - 1, column++, new QTableWidgetItem(*it));
                    it++;
                }
            }
            lineCounter++;
        }
    }
}

void MainWindow::LoadTagListIni()
{
    QString tagListIni = "./settings/tag_list.ini";
    QFile qfile(tagListIni);

    if (!qfile.exists())
        return;

    if (!qfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::information(ui->centralWidget, "Info", "\n" + tagListIni + " open failed!\n", "OK");
        return;
    }

    QTextStream in(&qfile);
    QStringList tagList;
    while(!in.atEnd()) {
        tagList.push_back(in.readLine());
    }

    if (!tagList.empty())
        ui->listWidgetTags->addItems(tagList);
}

void MainWindow::ffprobeVideoOutput()
{
    int column = 0;
    ui->tableWidgetVideoInfo->insertRow(ui->tableWidgetVideoInfo->rowCount());
    while (!ffprobeProcess->atEnd()) {
        char buffer[1024];
        QString strOutput;
        ffprobeProcess->readLine(buffer, 1024);
        strOutput = buffer;
        strOutput = strOutput.trimmed();
        if (strOutput == "[STREAM]" || strOutput == "[/STREAM]")
            continue;
        else {
            QStringList token = strOutput.split('=');
            QStringList::Iterator it = token.begin();
            while (it != token.end()) {
                if (*it == "codec_name") {
                    ui->tableWidgetVideoInfo->setItem(ui->tableWidgetVideoInfo->rowCount() - 1, column, new QTableWidgetItem("codec"));
                } else if (*it == "width" || *it  == "height") {
                    ui->tableWidgetVideoInfo->setItem(ui->tableWidgetVideoInfo->rowCount() - 1, column, new QTableWidgetItem(*it));
                } else if (*it == "bit_rate") {
                    ui->tableWidgetVideoInfo->setItem(ui->tableWidgetVideoInfo->rowCount() - 1, column, new QTableWidgetItem("bit rate"));
                } else if (*it == "avg_frame_rate") {
                    ui->tableWidgetVideoInfo->setItem(ui->tableWidgetVideoInfo->rowCount() - 1, column, new QTableWidgetItem("frame rate"));
                } else {
                    break;
                }

                it++;
                column = (column + 1) % 4;
                ui->tableWidgetVideoInfo->setItem(ui->tableWidgetVideoInfo->rowCount() - 1, column, new QTableWidgetItem(*it));
                column = (column + 1) % 4;
                it++;

                if (column == 0)
                    ui->tableWidgetVideoInfo->insertRow(ui->tableWidgetVideoInfo->rowCount());
            }
        }

        std::cerr << buffer;
    }
}

void MainWindow::ffprobeVideo()
{
    QFileInfo ffprobeInfo("./ffmpeg/bin/ffprobe.exe");
    if (!ffprobeInfo.exists()) {
        ffprobeMissing = true;
        QMessageBox::warning(ui->centralWidget, "Warning", "\nffprobe.exe is missing.\n", "OK");
        return;
    }

    ffprobeMissing = false;
    QString ffprobeCmd = "./ffmpeg/bin/ffprobe.exe -v quiet -print_format default -show_streams -select_streams video ";
    ffprobeCmd += ui->lineEditFileName->text();
    ui->tableWidgetVideoInfo->clearContents();
    ui->tableWidgetVideoInfo->setRowCount(0);
    ffprobeProcess->start(ffprobeCmd);
}

void MainWindow::CheckSelectedFile(QString s)
{
    if (!s.size()){
        QMessageBox::information(ui->centralWidget, "Information", "\nNo file was selected.\n", "OK");
    } else {
        ui->lineEditFileName->clear();
        ui->lineEditNewName->clear();
        ui->lineEditFileName->insert(s);
        mediaPlayer->setMedia(QUrl::fromLocalFile(s));
        //mediaPlayer->setMedia(QUrl("http://youtu.be/SSbfLI-CsJQ"));
        mediaPlayer->setVolume(30);
        mediaPlayer->play();

        delete fileInfo;
        fileInfo = new QFileInfo(s);
        CheckExistClipFile();
        ffprobeVideo();
        clipGenerated = false;
        ui->dockWidget->setWindowTitle(s);

        //mediaPlayer->meta
        //QMediaMetaData::VideoBitRate;
        //
        //mediaPlayer->setPlaybackRate(1);
        //videoWidget->
    }

    if (mediaPlayer->currentMedia().isNull()) {
        videoStatus->setText("NONE");
        audioStatus->setText("UNKNOWN");
        ui->toolButtonPlay->setEnabled(false);
        ui->timeEditBegin->setEnabled(false);
        ui->timeEditEnd->setEnabled(false);
        ui->toolButtonBeginSet->setEnabled(false);
        ui->toolButtonEndSet->setEnabled(false);
        ui->toolButtonClipAdd->setEnabled(false);
        ui->groupBoxTag->setEnabled(false);
        ui->listWidgetTags->setEnabled(false);
        ui->lineEditTagOther->setEnabled(false);
        ui->toolButtonBegin->setEnabled(false);
        ui->toolButtonEnd->setEnabled(false);
        ui->horizontalSliderVolume->setEnabled(false);
    } else {
        //ui->toolButtonPlay->setEnabled(true);
        ui->timeEditBegin->setEnabled(true);
        ui->timeEditEnd->setEnabled(true);
        ui->toolButtonBeginSet->setEnabled(true);
        ui->toolButtonEndSet->setEnabled(true);
        ui->toolButtonClipAdd->setEnabled(true);
        ui->groupBoxTag->setEnabled(true);
        ui->listWidgetTags->setEnabled(true);
        ui->lineEditTagOther->setEnabled(true);
        ui->toolButtonBegin->setEnabled(true);
        ui->toolButtonEnd->setEnabled(true);
        ui->horizontalSliderVolume->setEnabled(true);
    }
}

void MainWindow::on_toolButtonFileDialog_clicked()
{
    if (ui->tableWidget->rowCount() > 0 && !clipGenerated)
        if (QMessageBox::No == QMessageBox::question(ui->centralWidget, "Question", "\nCurrent clip file is not generated.\n\nDo you still want to select other file?\n"))
            return;

    QString s = QFileDialog::getOpenFileName(ui->centralWidget, "Select a video file",
                                             "", "Video Files (*.avi *.mp4 *.wmv *.mov *.rmvb);;All (*.*)");

    CheckSelectedFile(s);
}

void MainWindow::on_toolButtonPlay_clicked()
{
    if (mediaPlayer) {
        mediaPlayer->play();
    }
}

void MainWindow::on_toolButtonPause_clicked()
{
    if (mediaPlayer)
        mediaPlayer->pause();
}

void MainWindow::on_toolButtonStop_clicked()
{
    if (mediaPlayer) {
        mediaPlayer->stop();
    }
}

void MainWindow::mediaStateChanged(QMediaPlayer::State state)
{
    switch(state) {
    case QMediaPlayer::PlayingState:
        videoStatus->setText("PLAYING");
        ui->toolButtonStop->setEnabled(true);
        ui->toolButtonPause->setEnabled(true);
        ui->toolButtonPlay->setEnabled(false);
        break;
    case QMediaPlayer::PausedState:
        videoStatus->setText("PAUSED");
        ui->toolButtonStop->setEnabled(true);
        ui->toolButtonPause->setEnabled(false);
        ui->toolButtonPlay->setEnabled(true);
        break;
    case QMediaPlayer::StoppedState:
        videoStatus->setText("STOPPED");
        ui->toolButtonStop->setEnabled(false);
        ui->toolButtonPause->setEnabled(false);
        ui->toolButtonPlay->setEnabled(true);
        mediaPlayer->setMedia(QUrl::fromLocalFile(ui->lineEditFileName->text()));
        break;
    }
}

void MainWindow::setPosition(qint64 position)
{
    if (mediaPlayer) {
        switch(mediaPlayer->state()) {
        case QMediaPlayer::PlayingState:
            //mediaPlayer->pause();
            mediaPlayer->setPosition(position);
            //mediaPlayer->play();
            break;
        case QMediaPlayer::PausedState:
        case QMediaPlayer::StoppedState:
            mediaPlayer->setPosition(position);
            break;
        }
    }
}

void MainWindow::setPosition2(int position)
{
    disconnect(mediaPlayer, SIGNAL(positionChanged(qint64)),
            this, SLOT(positionChanged(qint64)));
    if (mediaPlayer) {
        mediaPlayer->setPosition(position);
    }
    QDateTime qDateTime = QDateTime::fromMSecsSinceEpoch(position);
    ui->timeEditCurrent->setTime(qDateTime.toUTC().time());
    connect(mediaPlayer, SIGNAL(positionChanged(qint64)),
            this, SLOT(positionChanged(qint64)));
}

void MainWindow::durationChanged(qint64 duration)
{
    QDateTime qDateTime = QDateTime::fromMSecsSinceEpoch(duration);
    ui->timeEditMax->setTime(qDateTime.toUTC().time());
    ui->horizontalSliderVideo->setRange(0, duration);
}

void MainWindow::positionChanged(qint64 position)
{
     QDateTime qDateTime = QDateTime::fromMSecsSinceEpoch(position);
     ui->timeEditCurrent->setTime(qDateTime.toUTC().time());

     disconnect(ui->horizontalSliderVideo, SIGNAL(valueChanged(int)),
                 this, SLOT(on_horizontalSliderVideo_valueChanged(int)));
     ui->horizontalSliderVideo->setValue(position);
     connect(ui->horizontalSliderVideo, SIGNAL(valueChanged(int)),
                 this, SLOT(on_horizontalSliderVideo_valueChanged(int)));
}

void MainWindow::metaDataAvailableChanged(bool available)
{
            //int counter = 0;
            //ui->listWidget->insertItem(counter++, "test");
    //mediaPlayer->metaDataAvailableChanged();
    //if (available) {
        //QMediaService *qMediaService = mediaPlayer->service();
        //QMetaDataReaderControl *qMetadataReader = qobject_cast<QMetaDataReaderControl *>(
        //            qMediaService->requestControl("org.qt-project.qt.metadatareadercontrol/5.0"));
           // QStringList qStrLst = mediaPlayer->availableMetaData();
           // QStringList::Iterator it = qStrLst.begin();
          //  while (it != qStrLst.end()) {
          //      ui->listWidget->insertItem(counter++, itoa(qStrLst.count(), NULL, 10));
          //  }
    //}
    //if (!available)
    //    std::cerr << "Metadata unavailable\n";
    //else
     //   std::cerr << "Metadata available\n";
}

void MainWindow::metaDataChanged()
{
  //  int counter = 4;
  //  ui->listWidget->insertItem(counter++, "test2");
    //mediaPlayer->metaData(QMediaMeta )
        //std::cerr << "Metadata changed\n";
}

void MainWindow::on_horizontalSliderVideo_valueChanged(int value)
{
    setPosition2(value);
}

void MainWindow::on_horizontalSliderVolume_sliderMoved(int position)
{
    //char buffer[10];
    //itoa(position, buffer, 10);
    //QString text(buffer);
    //audioStatus->setText(text);
    if (mediaPlayer)
        mediaPlayer->setVolume(position);
}

void MainWindow::on_toolButtonBeginSet_clicked()
{
    if (!ui->timeEditBegin->isEnabled())
        return;

    ui->timeEditBegin->setTime(ui->timeEditCurrent->time());
}

void MainWindow::on_toolButtonEndSet_clicked()
{
    if (!ui->timeEditEnd->isEnabled())
        return;

    ui->timeEditEnd->setTime(ui->timeEditCurrent->time());
}

void MainWindow::on_timeEditBegin_timeChanged(const QTime &time)
{
    CalulateClipLength();
}

void MainWindow::CalulateClipLength()
{
    QTime beginTime = ui->timeEditBegin->time();
    QTime endTime = ui->timeEditEnd->time();

    if (beginTime <= endTime) {
        ui->timeEditLength->setTime(QDateTime::fromMSecsSinceEpoch(beginTime.msecsTo(endTime)).toUTC().time());
    } else {
        ui->timeEditLength->setTime(QDateTime::fromMSecsSinceEpoch(0).toUTC().time());
    }
}

void MainWindow::on_timeEditEnd_userTimeChanged(const QTime &time)
{
    CalulateClipLength();
}

void MainWindow::on_toolButtonClipClear_clicked(bool checked)
{
    ui->timeEditBegin->setTime(QDateTime::fromMSecsSinceEpoch(0).toUTC().time());
    ui->timeEditEnd->setTime(QDateTime::fromMSecsSinceEpoch(0).toUTC().time());
    ui->timeEditLength->setTime(QDateTime::fromMSecsSinceEpoch(0).toUTC().time());
    ui->listWidgetTags->clearSelection();
    ui->lineEditTagOther->clear();
}

void MainWindow::on_toolButtonClipAdd_clicked()
{
    QString beginTime = ui->timeEditBegin->time().toString("hh:mm:ss");
    QString endTime = ui->timeEditEnd->time().toString("hh:mm:ss");
    QString length = ui->timeEditLength->time().toString("hh:mm:ss");

    if (beginTime.isNull() || endTime.isNull() || length.isNull())
        return;

    if (length == "00:00:00")
        return;

    int difference = ui->timeEditBegin->time().secsTo(ui->timeEditEnd->time());
    char strDifference[32];
    itoa(difference, strDifference, 10);

    for (int i = 0; i < ui->tableWidget->rowCount(); i++) {
        QTableWidgetItem *itemBegin = ui->tableWidget->item(i, 0);
        QTableWidgetItem *itemEnd = ui->tableWidget->item(i, 1);
        if (itemBegin->text() == beginTime && itemEnd->text() == endTime) {
            QTableWidgetItem *itemLength = ui->tableWidget->item(i, 2);
            QTableWidgetItem *itemDiff = ui->tableWidget->item(i, 3);

            if (itemLength->text() == length && itemDiff->text() == strDifference) {
                return;
            } else {
                QString msg("\nClip ");
                char buffer[32];
                itoa(i + 1, buffer, 10);
                msg += buffer;
                msg += "'s begin time and end time is the same as the added clip, but length and difference are different.\n\nStill add it into table?\n";
                if (QMessageBox::No == QMessageBox::question(ui->centralWidget, "Question", msg)) {
                    return;
                }
            }
        }
    }

    ui->tableWidget->insertRow(ui->tableWidget->rowCount());
    ui->tableWidget->setItem(ui->tableWidget->rowCount() - 1, 0, new QTableWidgetItem(beginTime));
    ui->tableWidget->setItem(ui->tableWidget->rowCount() - 1, 1, new QTableWidgetItem(endTime));
    ui->tableWidget->setItem(ui->tableWidget->rowCount() - 1, 2, new QTableWidgetItem(length));
    ui->tableWidget->setItem(ui->tableWidget->rowCount() - 1, 3, new QTableWidgetItem(strDifference));
    ui->tableWidget->setItem(ui->tableWidget->rowCount() - 1, 4, new QTableWidgetItem(ui->lineEditTagOther->text()));
}

void MainWindow::on_toolButtonTagAll_clicked()
{
    ui->listWidgetTags->selectAll();
}

void MainWindow::on_pushButtonReset_clicked()
{
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);
    ui->lineEditNewName->clear();
    on_toolButtonClipClear_clicked(false);
}

bool MainWindow::RenameVideoFile()
{
   if (ui->lineEditNewName->text().isEmpty())
       return true;

   bool sameVideoName = false;
   bool sameFolderName = false;
   QFileInfo currentFolder(fileInfo->absolutePath());
   QString oldFolderName = currentFolder.completeBaseName();
   QString oldVideoName = fileInfo->fileName();
   QString newFolderName = ui->lineEditNewName->text().trimmed();
   QString newVideoName = ui->lineEditNewName->text().trimmed() + "." + fileInfo->suffix();

   if (oldFolderName == newFolderName)
       sameFolderName = true;
   if (oldVideoName == newVideoName)
       sameVideoName = true;

   if (sameFolderName && sameVideoName)
       return true;

   if (mediaPlayer->state() == QMediaPlayer::PlayingState)
       mediaPlayer->stop();
   delete mediaPlayer;
   mediaPlayer = NULL;

   QString tempFullFilePath = fileInfo->absolutePath() + "/" + newVideoName;
   if (!sameVideoName) {
       QString oldFullFilePath = fileInfo->absoluteFilePath();

       std::cerr << oldFullFilePath.toStdString() << std::endl << tempFullFilePath.toStdString() << std::endl;
       if (!QFile::rename(oldFullFilePath, tempFullFilePath)) {
           InitializeMediaPlayer();
           mediaPlayer->setMedia(QUrl::fromLocalFile(oldFullFilePath));
           fileInfo = new QFileInfo(oldFullFilePath);

           if (QMessageBox::No == QMessageBox::question(ui->centralWidget, "Info", "\nRename Failed!\n\nStill try to generate clip file?\n")) {
               return false;
           } else {
               return true;
           }
       }
   }

   delete fileInfo;
   fileInfo = NULL;

   if (!sameFolderName) {
       QString upperFolder = currentFolder.absolutePath();
       QString newFullFolderPath = upperFolder + "/" + newFolderName;
       QString oldFullFolderPath = upperFolder + "/" + oldFolderName;
       QDir qdir(oldFullFolderPath);
       std::cerr << upperFolder.toStdString() << std::endl << newFullFolderPath.toStdString() << std::endl << oldFullFolderPath.toStdString() << std::endl;

       if (qdir.rename(oldFullFolderPath, newFullFolderPath)) {
           ui->lineEditFileName->setText(newFullFolderPath + "/" + newVideoName);
           InitializeMediaPlayer();
           mediaPlayer->setMedia(QUrl::fromLocalFile(ui->lineEditFileName->text()));
           fileInfo = new QFileInfo(ui->lineEditFileName->text());
       } else {
           ui->lineEditFileName->setText(tempFullFilePath);
           InitializeMediaPlayer();
           mediaPlayer->setMedia(QUrl::fromLocalFile(ui->lineEditFileName->text()));
           fileInfo = new QFileInfo(ui->lineEditFileName->text());
           if (QMessageBox::No == QMessageBox::question(ui->centralWidget, "Info", "\nRename Folder Failed!\n\nStill try to generate clip file?\n")) {
               return false;
           } else {
               return true;
           }
       }
   } else {
       ui->lineEditFileName->setText(tempFullFilePath);
       InitializeMediaPlayer();
       mediaPlayer->setMedia(QUrl::fromLocalFile(ui->lineEditFileName->text()));
       fileInfo = new QFileInfo(ui->lineEditFileName->text());
   }

   return true;
}

bool MainWindow::GenerateClipFile(QString &clipFileName)
{
    clipFileName = fileInfo->absolutePath() + "/" + fileInfo->completeBaseName() + ".clip";
    std::cerr << clipFileName.toStdString() << std::endl;
    QFile qfile(clipFileName);

    if (qfile.exists())
        qfile.remove();

    if (!qfile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        QMessageBox::information(ui->centralWidget, "Info", "\n" + clipFileName + " open failed!\n", "OK");
        return false;
    }

    QTextStream out(&qfile);
    out << fileInfo->fileName() <<"\n";
    out << ui->comboBox->currentText() << " -x264encopts bitrate=";
    if (ffprobeMissing) {
        out << "1000\n";
    } else {
        for (int i = 0; i < ui->tableWidgetVideoInfo->rowCount(); i++) {
            for (int j = 0; j < ui->tableWidgetVideoInfo->columnCount(); j++) {
                QTableWidgetItem *item = ui->tableWidgetVideoInfo->item(i, j);
                if (item->text() == "bit rate") {
                    item = ui->tableWidgetVideoInfo->item(i, j + 1);
                    if (item->text() == "N/A") {
                        out << "1000\n";
                        break;
                    } else {
                        long bitRate = item->text().toLong();
                        char buffer[32];
                        bitRate = bitRate / 1000 / 5 * 4;
                        if (bitRate < 1000)
                            out << "1000\n";
                        else {
                            ltoa(bitRate, buffer, 10);
                            out << buffer << "\n";
                        }
                        break;
                    }
                }
            }
        }
    }

    for (int i = 0; i < ui->tableWidget->rowCount(); i++) {
        for (int j = 0; j < ui->tableWidget->columnCount(); j++) {
            QTableWidgetItem *item = ui->tableWidget->item(i, j);
            if (item)
                out << item->text() << ' ';
        }
        out << "\n";
    }

    QMessageBox::information(ui->centralWidget, "Info", "\n" + clipFileName + " is generated!\n", "OK");
    qfile.close();
    return true;
}

void MainWindow::on_pushButtonVLGen_clicked()
{
    if (fileInfo) {
        if (!RenameVideoFile())
            return;

        QString clipFileName;
        if (!GenerateClipFile(clipFileName))
            return;

        QFile qfile("./" + ui->lineEditVideoList->text() + ".list");
        if (!qfile.open(QIODevice::ReadWrite | QIODevice::Text)) {
            QMessageBox::information(ui->centralWidget, "Info", "\n./" + ui->lineEditVideoList->text() + ".list" + " open failed!\n", "OK");
            return;
        }

        QTextStream out(&qfile);
        bool clipExist = false;
        while(!out.atEnd()) {
            QString line = out.readLine();
            if (line == clipFileName) {
                clipExist = true;
                break;
            }
        }

        if (!clipExist) {
            out << clipFileName << "\n";
        } else {
            QMessageBox::information(ui->centralWidget, "Info", "\n" + clipFileName + " exists!\n", "OK");
        }

        qfile.close();
    }

    clipGenerated = true;
}

void MainWindow::customContextMenuRequested(QPoint pos)
{
    QTableWidgetItem *item = ui->tableWidget->itemAt(pos);
    if (!item)
        return;

    QMenu menu(this);

    menu.addAction(new QAction("Delete", this));
    menu.addAction(new QAction("Play", this));

    menu.popup(ui->tableWidget->viewport()->mapToGlobal(pos));
    QAction *triggerAction = menu.exec();
    if(!triggerAction)
        return;

    if (triggerAction->text() == "Delete") {
        if (item) {
            ui->tableWidget->removeRow(item->row());
        }
    } else if (triggerAction->text() == "Play") {
        if (item) {
            if (item->column() < 2) {
                QTime qTime = QTime::fromString(item->text(), "hh:mm:ss");
                QTime qRefTime = QTime::fromString("00:00:00", "hh:mm:ss");
                qint64 msec = qRefTime.msecsTo(qTime);
                std::cerr << msec << std::endl;

                setPosition(msec);
                if (mediaPlayer->state() != QMediaPlayer::PlayingState)
                    mediaPlayer->play();
            }
        }
    }
}

void MainWindow::on_listWidgetTags_itemSelectionChanged()
{
    ui->lineEditTagOther->clear();

    QList<QListWidgetItem *> item = ui->listWidgetTags->selectedItems();
    QList<QListWidgetItem *>::Iterator it = item.begin();
    QString strTags;
    while (it != item.end()) {
        strTags += (*it)->text();
        strTags += '_';
        it++;
    }

    ui->lineEditTagOther->setText(strTags.remove(strTags.lastIndexOf('_'), 1));
}

void MainWindow::on_toolButton_clicked()
{
    ui->listWidgetTags->clear();
    LoadTagListIni();
}

void MainWindow::on_toolButtonBegin_clicked()
{
    on_toolButtonBeginSet_clicked();
}

void MainWindow::on_toolButtonEnd_clicked()
{
    on_toolButtonEndSet_clicked();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        if (urlList.empty())
            return;
        QString file = urlList.at(0).toLocalFile();
        QFileInfo info(file);
        QString suffix = info.suffix().toCaseFolded();
        if (suffix == "avi" || suffix == "mp4" || suffix == "wmv" ||
                suffix == "mov" || suffix == "rmvb" || suffix == "rm") {
            CheckSelectedFile(file);
        } else if (suffix == "list") {
            ui->lineEditVideoList->setText(info.baseName());
        }
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    unsigned modifier = event->modifiers();
        switch (event->key()) {
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            ui->horizontalSliderVideo->event(event);
            return;
        case Qt::Key_Left:
        case Qt::Key_Right:
            ui->horizontalSliderVolume->event(event);
            return;
        case Qt::Key_S:
            if (Qt::AltModifier == modifier) {
                on_toolButtonBeginSet_clicked();
                return;
            }
        case Qt::Key_E:
            if (Qt::AltModifier == modifier) {
                on_toolButtonEndSet_clicked();
                return;
            }
        case Qt::Key_A:
            if (Qt::AltModifier == modifier) {
                on_toolButtonClipAdd_clicked();
                return;
            }
        case Qt::Key_F:
            if (Qt::AltModifier == modifier) {
                ui->centralWidget->setFocus();
                return;
            }
        case Qt::Key_M:
            if (mediaPlayer->currentMedia().isNull())
                return;
            if (Qt::AltModifier == modifier) {
                if (mediaPlayer->isMuted()) {
                    mediaPlayer->setMuted(false);
                    char buffer[10];
                    itoa(mediaPlayer->volume(), buffer, 10);
                    QString text(buffer);
                    audioStatus->setText(text);
                } else {
                    audioStatus->setText("MUTED");
                    mediaPlayer->setMuted(true);
                }
            }
        case Qt::Key_Enter:
            if (Qt::AltModifier == modifier) {
                if (videoWidget->isFullScreen()) {
                    videoWidget->setFullScreen(false);
                } else {
                    videoWidget->setFullScreen(true);
                }
            }
        }
}

void MainWindow::on_pushButtonUpdate_clicked()
{
    QString clipFileName;
    GenerateClipFile(clipFileName);
}

void MainWindow::volumeChanged(int volume)
{
    if (!mediaPlayer->isMuted()) {
        char buffer[10];
        itoa(volume, buffer, 10);
        QString text(buffer);
        audioStatus->setText(text);
    } else {
        audioStatus->setText("MUTED");
    }

    disconnect(ui->horizontalSliderVolume, SIGNAL(valueChanged(int)),
                this, SLOT(on_horizontalSliderVolume_valueChanged(int)));
    ui->horizontalSliderVolume->setValue(volume);
    connect(ui->horizontalSliderVolume, SIGNAL(valueChanged(int)),
                this, SLOT(on_horizontalSliderVolume_valueChanged(int)));
}

void MainWindow::on_horizontalSliderVolume_valueChanged(int value)
{
    on_horizontalSliderVolume_sliderMoved(value);
}

void MainWindow::on_dockWidget_dockLocationChanged(const Qt::DockWidgetArea &area)
{
    if (area == Qt::LeftDockWidgetArea || area == Qt::RightDockWidgetArea) {
        const QRect rect = geometry();
        setGeometry(rect.x(), rect.y(), 1572, rect.height());
        setMinimumWidth(1572);
    }
}

void MainWindow::on_dockWidget_featuresChanged(const QDockWidget::DockWidgetFeatures &features)
{
    std::cout << "features changed" << std::endl;
}

void MainWindow::on_dockWidget_topLevelChanged(bool topLevel)
{
    if (topLevel) {
        const QRect rect = geometry();
        setMinimumWidth(615);
        setMaximumWidth(615);
        setGeometry(rect.x(), rect.y(), 615, rect.height());
    } else {
        setMaximumWidth(1572);
    }
}
