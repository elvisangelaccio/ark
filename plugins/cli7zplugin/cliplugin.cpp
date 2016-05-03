/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "cliplugin.h"
#include "ark_debug.h"
#include "kerfuffle/cliinterface.h"
#include "kerfuffle/kerfuffle_export.h"

#include <QDateTime>
#include <QDir>
#include <QRegularExpression>

#include <KPluginFactory>

using namespace Kerfuffle;

K_PLUGIN_FACTORY_WITH_JSON(CliPluginFactory, "kerfuffle_cli7z.json", registerPlugin<CliPlugin>();)

CliPlugin::CliPlugin(QObject *parent, const QVariantList & args)
        : CliInterface(parent, args)
        , m_archiveType(ArchiveType7z)
        , m_parseState(ParseStateTitle)
        , m_linesComment(0)
{
    qCDebug(ARK) << "Loaded cli_7z plugin";
}

CliPlugin::~CliPlugin()
{
}

void CliPlugin::resetParsing()
{
    m_parseState = ParseStateTitle;
    m_comment.clear();
}

ParameterList CliPlugin::parameterList() const
{
    static ParameterList p;

    if (p.isEmpty()) {
        //p[CaptureProgress] = true;
        p[ListProgram] = p[ExtractProgram] = p[DeleteProgram] = p[AddProgram] = QStringList() << QStringLiteral("7z");
        p[ListArgs] = QStringList() << QStringLiteral("l")
                                    << QStringLiteral("-slt")
                                    << QStringLiteral("$PasswordSwitch")
                                    << QStringLiteral("$Archive");
        p[ExtractArgs] = QStringList() << QStringLiteral("$PreservePathSwitch")
                                       << QStringLiteral("$PasswordSwitch")
                                       << QStringLiteral("$Archive")
                                       << QStringLiteral("$Files");
        p[PreservePathSwitch] = QStringList() << QStringLiteral("x")
                                              << QStringLiteral("e");
        p[PasswordSwitch] = QStringList() << QStringLiteral("-p$Password");
        p[EncryptHeaderSwitch] = QStringList() << QStringLiteral("$Enabled");
        p[WrongPasswordPatterns] = QStringList() << QStringLiteral("Wrong password");
        p[AddArgs] = QStringList() << QStringLiteral("a")
                                   << QStringLiteral("$Archive")
                                   << QStringLiteral("$EncryptHeaderSwitch")
                                   << QStringLiteral("$PasswordSwitch")
                                   << QStringLiteral("$Files");
        p[DeleteArgs] = QStringList() << QStringLiteral("d")
                                      << QStringLiteral("$PasswordSwitch")
                                      << QStringLiteral("$Archive")
                                      << QStringLiteral("$Files");

        p[FileExistsExpression] = QStringList()
            << QStringLiteral("^\\(Y\\)es / \\(N\\)o / \\(A\\)lways / \\(S\\)kip all / A\\(u\\)to rename all / \\(Q\\)uit\\? $")
            << QStringLiteral("^\\? \\(Y\\)es / \\(N\\)o / \\(A\\)lways / \\(S\\)kip all / A\\(u\\)to rename all / \\(Q\\)uit\\? $");
        p[FileExistsFileName] = QStringList() << QStringLiteral("^file \\./(.*)$")
                                              << QStringLiteral("^  Path:     \\./(.*)$");
        p[FileExistsInput] = QStringList() << QStringLiteral("Y")  //overwrite
                                           << QStringLiteral("N")  //skip
                                           << QStringLiteral("A")  //overwrite all
                                           << QStringLiteral("S")  //autoskip
                                           << QStringLiteral("Q"); //cancel
        p[PasswordPromptPattern] = QStringLiteral("Enter password \\(will not be echoed\\)");
        p[ExtractionFailedPatterns] = QStringList() << QStringLiteral("ERROR: E_FAIL");
        p[CorruptArchivePatterns] = QStringList() << QStringLiteral("Unexpected end of archive")
                                                  << QStringLiteral("Headers Error");
        p[DiskFullPatterns] = QStringList() << QStringLiteral("No space left on device");
    }

    return p;
}

bool CliPlugin::readListLine(const QString& line)
{
    static const QLatin1String archiveInfoDelimiter1("--"); // 7z 9.13+
    static const QLatin1String archiveInfoDelimiter2("----"); // 7z 9.04
    static const QLatin1String entryInfoDelimiter("----------");
    const QRegularExpression rxComment(QStringLiteral("Comment = .+$"));

    if (m_parseState == ParseStateTitle) {

        const QRegularExpression rxVersionLine(QStringLiteral("^p7zip Version ([\\d\\.]+) .*$"));
        QRegularExpressionMatch matchVersion = rxVersionLine.match(line);
        if (matchVersion.hasMatch()) {
            m_parseState = ParseStateHeader;
            const QString p7zipVersion = matchVersion.captured(1);
            qCDebug(ARK) << "p7zip version" << p7zipVersion << "detected";
        }

    } else if (m_parseState == ParseStateHeader) {

        if (line.startsWith(QStringLiteral("Listing archive:"))) {
            qCDebug(ARK) << "Archive name: "
                     << line.right(line.size() - 16).trimmed();
        } else if ((line == archiveInfoDelimiter1) ||
                   (line == archiveInfoDelimiter2)) {
            m_parseState = ParseStateArchiveInformation;
        } else if (line.contains(QStringLiteral("Error: "))) {
            qCWarning(ARK) << line.mid(7);
        }

    } else if (m_parseState == ParseStateArchiveInformation) {

        if (line == entryInfoDelimiter) {
            m_parseState = ParseStateEntryInformation;
        } else if (line.startsWith(QStringLiteral("Type = "))) {
            const QString type = line.mid(7).trimmed();
            qCDebug(ARK) << "Archive type: " << type;

            if (type == QLatin1String("7z")) {
                m_archiveType = ArchiveType7z;
            } else if (type == QLatin1String("bzip2")) {
                m_archiveType = ArchiveTypeBZip2;
            } else if (type == QLatin1String("gzip")) {
                m_archiveType = ArchiveTypeGZip;
            } else if (type == QLatin1String("xz")) {
                m_archiveType = ArchiveTypeXz;
            } else if (type == QLatin1String("tar")) {
                m_archiveType = ArchiveTypeTar;
            } else if (type == QLatin1String("zip")) {
                m_archiveType = ArchiveTypeZip;
            } else if (type == QLatin1String("Rar")) {
                m_archiveType = ArchiveTypeRar;
            } else {
                // Should not happen
                qCWarning(ARK) << "Unsupported archive type";
                return false;
            }

        } else if (rxComment.match(line).hasMatch()) {
            m_parseState = ParseStateComment;
            m_comment.append(line.section(QLatin1Char('='), 1) + QLatin1Char('\n'));
        }

    } else if (m_parseState == ParseStateComment) {

        if (line == entryInfoDelimiter) {
            m_parseState = ParseStateEntryInformation;
            if (!m_comment.trimmed().isEmpty()) {
                m_comment = m_comment.trimmed();
                m_linesComment = m_comment.count(QLatin1Char('\n')) + 1;
                qCDebug(ARK) << "Found a comment with" << m_linesComment << "lines";
            }
        } else {
            m_comment.append(line + QLatin1Char('\n'));
        }

    } else if (m_parseState == ParseStateEntryInformation) {

        if (line.startsWith(QStringLiteral("Path = "))) {
            const QString entryFilename =
                QDir::fromNativeSeparators(line.mid(7).trimmed());
            m_currentArchiveEntry.clear();
            m_currentArchiveEntry[FileName] = entryFilename;
            m_currentArchiveEntry[InternalID] = entryFilename;
        } else if (line.startsWith(QStringLiteral("Size = "))) {
            m_currentArchiveEntry[ Size ] = line.mid(7).trimmed();
        } else if (line.startsWith(QStringLiteral("Packed Size = "))) {
            // #236696: 7z files only show a single Packed Size value
            //          corresponding to the whole archive.
            if (m_archiveType != ArchiveType7z) {
                m_currentArchiveEntry[CompressedSize] = line.mid(14).trimmed();
            }
        } else if (line.startsWith(QStringLiteral("Modified = "))) {
            m_currentArchiveEntry[ Timestamp ] =
                QDateTime::fromString(line.mid(11).trimmed(),
                                      QStringLiteral("yyyy-MM-dd hh:mm:ss"));
        } else if (line.startsWith(QStringLiteral("Attributes = "))) {
            const QString attributes = line.mid(13).trimmed();

            const bool isDirectory = attributes.startsWith(QLatin1Char('D'));
            m_currentArchiveEntry[ IsDirectory ] = isDirectory;
            if (isDirectory) {
                const QString directoryName =
                    m_currentArchiveEntry[FileName].toString();
                if (!directoryName.endsWith(QLatin1Char('/'))) {
                    const bool isPasswordProtected = (line.at(12) == QLatin1Char('+'));
                    m_currentArchiveEntry[FileName] =
                        m_currentArchiveEntry[InternalID] = QString(directoryName + QLatin1Char('/'));
                    m_currentArchiveEntry[ IsPasswordProtected ] =
                        isPasswordProtected;
                }
            }

            m_currentArchiveEntry[ Permissions ] = attributes.mid(1);
        } else if (line.startsWith(QStringLiteral("CRC = "))) {
            m_currentArchiveEntry[ CRC ] = line.mid(6).trimmed();
        } else if (line.startsWith(QStringLiteral("Method = "))) {
            m_currentArchiveEntry[ Method ] = line.mid(9).trimmed();
        } else if (line.startsWith(QStringLiteral("Encrypted = ")) &&
                   line.size() >= 13) {
            m_currentArchiveEntry[ IsPasswordProtected ] = (line.at(12) == QLatin1Char('+'));
        } else if (line.startsWith(QStringLiteral("Block = ")) ||
                   line.startsWith(QStringLiteral("Version = "))) {
            if (m_currentArchiveEntry.contains(FileName)) {
                emit entry(m_currentArchiveEntry);
            }
        }
    }

    return true;
}

#include "cliplugin.moc"
