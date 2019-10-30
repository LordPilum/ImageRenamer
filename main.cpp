#include <QCoreApplication>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QImage>
#include <QDateTime>
#include <QRegularExpression>
#include <QDebug>
#include "exif.h"

QString readExifDate(QString fileName)
{
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


QString formatDateString(const QString& dateString)
{
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

bool fileExists(const QString& path)
{
	QFileInfo file(path);

	if (file.exists() && file.isFile()) {
		return true;
	} else {
		return false;
	}
}

QString fileNameSetDuplicateCounter(const QString &fileBaseName)
{
	auto workingFileBaseName = fileBaseName;
	qDebug() << "Attempting to rename " + workingFileBaseName;

	// Trying to match an existing duplicate counter.
	QRegularExpression pattern("(.+)(\\()(\\d+)(\\))$");
	auto match = pattern.match(workingFileBaseName);

	if(!match.hasMatch())
		qDebug() << "No match.";

	if(!match.hasMatch())
		// No counter found. Starting at 0.
		return fileBaseName + "(0)";

	// Get and increment the counter.
	auto counterStr = match.captured(2);
	auto counter = counterStr.toInt();
	++counter;

	qDebug() << "Duplicate counter value: " + QString::number(counter);

	// Set and return the new file base name.
	return workingFileBaseName.replace(pattern, "\\1\\2" + QString::number(counter) + "\\4");
}

void renameFile(const QFileInfo& file)
{
	// Getting the date string which will be the basis for the new filename
	// from the image's exif data.
	auto dateString = readExifDate(file.absoluteFilePath());

	// Attempt a rename only if the date string is not empty.
	if(!dateString.isEmpty()) {
		// Set the component parts of the file's absolute path.
		auto fileBaseName = formatDateString(dateString);
		auto fileExtensionStr =  "." + file.suffix();
		auto path = file.absolutePath() + QDir::separator();

		auto newAbsoluteFilePath = path + fileBaseName + fileExtensionStr;

		// Do not rename the file if the new filename is the same.
		if(newAbsoluteFilePath != file.absoluteFilePath()) {
			// Adding a counter to the base file name if a file with
			// that name already exists.
			while(fileExists(newAbsoluteFilePath))
			{
				qDebug() << "File exists. Setting a duplicate counter suffix.";
				fileBaseName = fileNameSetDuplicateCounter(fileBaseName);
				newAbsoluteFilePath = path + fileBaseName + fileExtensionStr;
				qDebug() << "New path: " + newAbsoluteFilePath;
			}

			QFile::rename(file.absoluteFilePath(), newAbsoluteFilePath);
		}
	}
}

void createThumbnail(const QFileInfo& file)
{
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


void renameFilesInDirectory(const QDir& directory)
{
	QFileInfoList fileList = directory.entryInfoList();

	for(int i = 0; i < fileList.size(); ++i) {
		QFileInfo fileInfo = fileList.at(i);
		QString ext = fileInfo.suffix().toLower();

		qDebug() << fileInfo.created().toString("yyyy:MM:dd HH:mm:ss");
		if(ext == "jpg" || ext == "jpeg") {
			renameFile(fileInfo);
		}
	}
}


void thumbnailFilesInDirectory(const QDir& directory)
{
	QDir thumbDir(directory.path() + QDir::separator() + "thumbs");

	if(!thumbDir.exists()) {
		directory.mkdir("thumbs");
	}

	QFileInfoList fileList = directory.entryInfoList();

	for(int i = 0; i < fileList.size(); ++i) {
		QFileInfo fileInfo = fileList.at(i);
		QString ext = fileInfo.suffix().toLower();

		if(ext == "jpg" || ext == "jpeg") {
			createThumbnail(fileInfo);
		}
	}
}


int main(int argc, char *argv[])
{
	QCoreApplication app(argc, argv);

	if(argc > 2)
	{
		qWarning() << "This application takes a directory path as its only argument. Defaults to the current directory.";
		return 1;
	}

	// Setting the application's operating path.
	QString directoryPath;
	if(argc == 2)
		directoryPath = QCoreApplication::arguments().takeAt(1);
	else
		directoryPath = "./";

	QDir directory(directoryPath);

	// Verifying that the directory exists before proceeding.
	if(!directory.exists())
	{
		qWarning() << "Directory " << directory << " does not exist.";
		return 2;
	}

	// We only want to access readable and writable files.
	directory.setFilter(QDir::Files | QDir::Readable | QDir::Writable | QDir::NoSymLinks);

	// Rename and create thumbnails.
	renameFilesInDirectory(directory);
	thumbnailFilesInDirectory(directory);

	return 0;
}
