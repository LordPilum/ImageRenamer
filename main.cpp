#include <QCoreApplication>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QImage>
#include <QDateTime>
#include <QDebug>
#include "exif.h"

QString readExifDate(QString fileName) {
	QString dateStr;
	QByteArray blob;
	unsigned char* buf;
	EXIFInfo result;


	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly)) return "-1";
	blob = file.readAll();
	buf = (unsigned char*) blob.constData();


	// Parse exif
	ParseEXIF(buf, blob.size(), result);


	if(result.dateTimeOriginal) {
		dateStr = result.dateTimeOriginal;
	}


	return dateStr;
}


QString formatDateString(const QString& dateString) {
	QStringList dateTimeElements;
	QStringList dateElements;
	QStringList timeElements;
	QString joinChar;
	QString formattedDateString;

	dateTimeElements = dateString.split(" ", QString::KeepEmptyParts);
	dateElements = dateTimeElements[0].split(':', QString::SkipEmptyParts);
	timeElements = dateTimeElements[1].split(':', QString::SkipEmptyParts);
	joinChar = "";

	formattedDateString = dateElements.join(joinChar) + "_" + timeElements.join(joinChar);

	return formattedDateString;
}


void renameFile(const QFileInfo& file) {
	QString dateString;
	QString newAbsoluteFilePath;

	dateString = readExifDate(file.absoluteFilePath());

	if(!dateString.isEmpty()) {
		newAbsoluteFilePath = file.absolutePath() + QDir::separator() + formatDateString(dateString) + "." + file.suffix();

		if(newAbsoluteFilePath != file.absoluteFilePath()) {
			QFile::rename(file.absoluteFilePath(), newAbsoluteFilePath);
		}
	}
}


void createThumbnail(const QFileInfo& file) {
	QString thumbnailFilePath;
	int height;

	QImage img(file.absoluteFilePath());
	QImage scaledImg;

	height = 120;
	thumbnailFilePath = file.absolutePath() + QDir::separator() + "thumbs" + QDir::separator() + file.baseName() + "_thumb." + file.suffix().toLower();

	qDebug() << thumbnailFilePath;

	scaledImg = img.scaledToHeight(height, Qt::SmoothTransformation);
	scaledImg.save(thumbnailFilePath);
}


void renameFilesInDirectory(const QString& directory) {
	QDir dir(directory);

	if(dir.exists()) {
		dir.setFilter(QDir::Files | QDir::Readable | QDir::Writable | QDir::NoSymLinks);

		QFileInfoList fileList = dir.entryInfoList();

		for(int i = 0; i < fileList.size(); ++i) {
			QFileInfo fileInfo = fileList.at(i);
			QString ext = fileInfo.suffix().toLower();
qDebug() << fileInfo.absoluteFilePath();
			qDebug() << fileInfo.created().toString("yyyy:MM:dd HH:mm:ss");
			if(ext == "jpg" || ext == "jpeg") {
				renameFile(fileInfo);
			}
		}
	} else {
		qWarning() << "Directory " << directory << " does not exist.";
	}
}


void thumbnailFilesInDirectory(const QString& directory) {
	QDir dir(directory);
	QDir thumbDir(directory + QDir::separator() + "thumbs");

	if(dir.exists()) {
		if(!thumbDir.exists()) {
			dir.mkdir("thumbs");
		}

		dir.setFilter(QDir::Files | QDir::Readable | QDir::Writable | QDir::NoSymLinks);

		QFileInfoList fileList = dir.entryInfoList();

		for(int i = 0; i < fileList.size(); ++i) {
			QFileInfo fileInfo = fileList.at(i);
			QString ext = fileInfo.suffix().toLower();

			if(ext == "jpg" || ext == "jpeg") {
				createThumbnail(fileInfo);
			}
		}

	} else {
		qWarning() << "Directory " << directory << " does not exist.";
	}
}


int main(int argc, char *argv[]) {
	QCoreApplication a(argc, argv);

//	renameFilesInDirectory(argv[1]);
	renameFilesInDirectory("./");
//	thumbnailFilesInDirectory(argv[1]);
	thumbnailFilesInDirectory("./");

    return a.exec();
}
