/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "archivebase.h"
#include "internaljobs.h"
#include "settings.h"

#include <kdebug.h>
#include <ThreadWeaver/Job>
#include <ThreadWeaver/Weaver>

#include <QFile>
#include <QDir>
#include <QList>
#include <QStringList>
#include <QDateTime>

ArchiveBase::ArchiveBase( ReadOnlyArchiveInterface *archive )
	: Arch( archive->filename()  ), m_iface( archive )
{
	Q_ASSERT( archive );
	archive->setParent( this );
	setReadOnly( archive->isReadOnly() );
}

ArchiveBase::~ArchiveBase()
{
	delete m_iface;
	m_iface = 0;
}

void ArchiveBase::open()
{
	ListingJob *job = new ListingJob( m_iface, this );

	connect( job, SIGNAL( done( ThreadWeaver::Job* ) ),
	         this, SLOT( listingDone( ThreadWeaver::Job * ) ) );
	connect( job, SIGNAL( entry( const ArchiveEntry & ) ),
	         this, SIGNAL( newEntry( const ArchiveEntry & ) ) );
	connect( job, SIGNAL( error( const QString&, const QString& ) ),
	         this, SIGNAL( error( const QString&, const QString& ) ) );
	ThreadWeaver::Weaver::instance()->enqueue( job );
}

void ArchiveBase::listingDone( ThreadWeaver::Job *job )
{
	emit opened( job->success() );
	delete job;
}

void ArchiveBase::create()
{
}

void ArchiveBase::addFile( const QStringList & )
{
}

void ArchiveBase::addDir( const QString & )
{
}

void ArchiveBase::remove( const QStringList & )
{
}

void ArchiveBase::extractFiles( const QList<QVariant> & files, const QString& destinationDir, bool preservePaths )
{
	ExtractionJob *job = new ExtractionJob( m_iface, files, destinationDir, preservePaths, this );
	connect( job, SIGNAL( done( ThreadWeaver::Job* ) ),
	         this, SLOT( extractionDone( ThreadWeaver::Job * ) ) );
	connect( job, SIGNAL( error( const QString&, const QString& ) ),
	         this, SIGNAL( error( const QString&, const QString& ) ) );
	ThreadWeaver::Weaver::instance()->enqueue( job );
}

void ArchiveBase::extractionDone( ThreadWeaver::Job *job )
{
	emit sigExtract( job->success() );
	delete job;
}


#include "archivebase.moc"
