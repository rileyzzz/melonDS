#ifndef ARCHIVEUTIL_H
#define ARCHIVEUTIL_H

#include <stdio.h>

#include <string>
#include <memory>

#include <QVector>
#include <QDir>

#ifdef ARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif

#include "types.h"

namespace Archive
{
    
    QVector<QString> ListArchive(const char* path);
    QVector<QString> ExtractFileFromArchive(const char* path, const char* wantedFile, QByteArray* romBuffer);
    //{
    //    return QVector<QString>();
    //}
    u32 ExtractFileFromArchive(const char* path, const char* wantedFile, u8** romdata);

}

#endif // ARCHIVEUTIL_H
