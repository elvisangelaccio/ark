/*

  ark -- archiver for the KDE project

  Copyright (C)

  2005: Henrique Pinto <henrique.pinto@kdemail.net>
  2003: Georg Robbers <Georg.Robbers@urz.uni-hd.de>
  2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
  1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
  1999: Francois-Xavier Duranceau duranceau@kde.org

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// Qt includes
#include <qpainter.h>
#include <qwhatsthis.h>

// KDE includes
#include <klocale.h>
#include <kglobal.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kmimetype.h>

#include "filelistview.h"
#include "arch.h"

/////////////////////////////////////////////////////////////////////
// FileLVI implementation
/////////////////////////////////////////////////////////////////////

QString FileLVI::key( int column, bool ascending ) const
{
	if ( column == 0 )
		return fileName();
	else
		return QListViewItem::key( column, ascending );
}

int FileLVI::compare( QListViewItem * i, int column, bool ascending ) const
{
	if ( column == 0 )
		return KListViewItem::compare( i, column, ascending );

	FileLVI * item = static_cast< FileLVI * >( i );
	columnName colName = ( static_cast< FileListView * > ( listView() ) )->nameOfColumn( column );
	switch ( colName )
	{
		case sizeCol:
		{
			return ( m_fileSize < item->fileSize() ? -1 :
			           ( m_fileSize > item->fileSize() ? 1 : 0 )
			       );
			break;
		}

		case ratioStrCol:
		{
			return ( m_ratio < item->ratio() ? -1 :
			           ( m_ratio > item->ratio() ? 1 : 0 )
			       );
			break;
		}

		case packedStrCol:
		{
			return ( m_packedFileSize < item->packedFileSize() ? -1 :
			           ( m_packedFileSize > item->packedFileSize() ? 1 : 0 )
			       );
			break;
		}

		case timeStampStrCol:
		{
			return ( m_timeStamp < item->timeStamp() ? -1 :
			           ( m_timeStamp > item->timeStamp() ? 1 : 0 )
			       );
			break;
		}

		default:
			return KListViewItem::compare( i, column, ascending );
	}
}

void FileLVI::setText( int column, const QString &text )
{
	columnName colName = ( static_cast< FileListView * > ( listView() ) )->nameOfColumn( column );
	if ( column == 0 )
	{
		QString name = text;
		if ( name.endsWith( "/" ) )
			name = name.left( name.length() - 1 );
		int pos = name.findRev( '/' );
		if ( pos != -1 )
			name = name.right( name.length() - pos - 1 );
		QListViewItem::setText( column, name );
		m_entryName = text;
	}
	else if ( colName == sizeCol )
	{
		m_fileSize = text.toLong();
		QListViewItem::setText( column, KIO::convertSize( m_fileSize ) );
	}
	else if ( colName == packedStrCol )
	{
		m_packedFileSize = text.toLong();
		QListViewItem::setText( column, KIO::convertSize( m_packedFileSize ) );
	}
	else if ( colName == ratioStrCol )
	{
		int l = text.length() - 1;
		if ( l>0 && text[l] == '%' )
			m_ratio = text.left(l).toDouble();
		else
			m_ratio = text.toDouble();
		QListViewItem::setText( column, i18n( "Packed Ratio", "%1 %" )
		                                .arg(KGlobal::locale()->formatNumber( m_ratio, 1 ) )
		                      );
	}
	else if ( colName == timeStampStrCol )
	{
		m_timeStamp = QDateTime::fromString( text, ISODate );
		QListViewItem::setText( column, KGlobal::locale()->formatDateTime( m_timeStamp ) );
	}
	else
		QListViewItem::setText(column, text);
}

/////////////////////////////////////////////////////////////////////
// FileListView implementation
/////////////////////////////////////////////////////////////////////


FileListView::FileListView(QWidget *parent, const char* name)
	: KListView(parent, name)
{
	QWhatsThis::add( this,
	                 i18n( "This area is for displaying information about the files contained within an archive." )
	               );

	setMultiSelection( true );
	setSelectionModeExt( FileManager );
	setDragEnabled( true );
	setItemsMovable( false );
	setRootIsDecorated( true );


	m_bPressed = false;
}

int FileListView::addColumn ( const QString & label, int width )
{
	int index = KListView::addColumn( label, width );
	if ( label == SIZE_COLUMN.first )
	{
		colMap[ index ] = sizeCol;
	}
	else if ( label == PACKED_COLUMN.first )
	{
		colMap[ index ] = packedStrCol;
	}
	else if ( label == RATIO_COLUMN.first )
	{
		colMap[ index ] = ratioStrCol;
	}
	else if ( label == TIMESTAMP_COLUMN.first )
	{
		colMap[ index ] = timeStampStrCol;
	}
	else
	{
		colMap[ index ] = otherCol;
	}
	return index;
}

void FileListView::removeColumn( int index )
{
	for ( unsigned int i = index; i < colMap.count() - 2; i++ )
	{
		colMap.replace( i, colMap[ i + 1 ] );
	}

	colMap.remove( colMap[ colMap.count() - 1 ] );
	KListView::removeColumn( index );
}

columnName FileListView::nameOfColumn( int index )
{
	return colMap[ index ];
}

QStringList FileListView::selectedFilenames()
{
	QStringList files;

	QListViewItemIterator it( this, QListViewItemIterator::Selected );
	while ( it.current() )
	{
		FileLVI *item = static_cast<FileLVI*>( it.current() );
		files += item->fileName();
		++it;
	}

	return files;
}

QStringList FileListView::fileNames()
{
	QStringList files;

	QListViewItemIterator it( this );
	while ( it.current() )
	{
		FileLVI *item = static_cast<FileLVI*>( it.current() );
		files += item->fileName();
		++it;
	}

	return files;
}

bool FileListView::isSelectionEmpty()
{
	FileLVI * flvi = (FileLVI*)firstChild();

	while (flvi)
	{
		if( flvi->isSelected() )
			return false;
		else
		flvi = (FileLVI*)flvi->itemBelow();
	}
	return true;
}

void
FileListView::contentsMousePressEvent(QMouseEvent *e)
{
	if( e->button()==QMouseEvent::LeftButton )
	{
		m_bPressed = true;
		presspos = e->pos();
	}

	KListView::contentsMousePressEvent(e);
}

void
FileListView::contentsMouseReleaseEvent(QMouseEvent *e)
{
	m_bPressed = false;
	KListView::contentsMouseReleaseEvent(e);
}

void
FileListView::contentsMouseMoveEvent(QMouseEvent *e)
{
	if(!m_bPressed)
	{
		KListView::contentsMouseMoveEvent(e);
	}
	else if( ( presspos - e->pos() ).manhattanLength() > KGlobalSettings::dndEventDelay() )
	{
		m_bPressed = false;	// Prevent triggering again
		if(isSelectionEmpty())
		{
			return;
		}
		QStringList dragFiles = selectedFilenames();
		emit startDragRequest( dragFiles );
		KListView::contentsMouseMoveEvent(e);
	}
}

FileLVI*
FileListView::item(const QString& filename) const
{
	FileLVI * flvi = (FileLVI*) firstChild();

	while (flvi)
	{
		QString curFilename = flvi->fileName();
		if (curFilename == filename)
			return flvi;
		flvi = (FileLVI*) flvi->nextSibling();
	}

	return 0;
}

void FileListView::addItem( const QStringList & entries )
{
	FileLVI *flvi, *parent = findParent( entries[0] );
	if ( parent )
		flvi = new FileLVI( parent );
	else
		flvi = new FileLVI( this );


	int i = 0;

	for (QStringList::ConstIterator it = entries.begin(); it != entries.end(); ++it)
	{
		flvi->setText(i, *it);
		++i;
	}

	KMimeType::Ptr mimeType = KMimeType::findByPath( entries.first(), 0, true );
	flvi->setPixmap( 0, mimeType->pixmap( KIcon::Small ) );
}

void FileListView::selectAll()
{
	QListView::selectAll( true );
}

void FileListView::unselectAll()
{
	QListView::selectAll( false );
}

void FileListView::setHeaders( const ColumnList& columns )
{
	clearHeaders();

	for ( ColumnList::const_iterator it = columns.constBegin();
	      it != columns.constEnd();
	      ++it )
	{
		QPair< QString, Qt::AlignmentFlags > pair = *it;
		int colnum = addColumn( pair.first );
		setColumnAlignment( colnum, pair.second );
	}
}

void FileListView::clearHeaders()
{
	while ( columns() > 0 )
	{
		removeColumn( 0 );
	}
}

int FileListView::totalFiles()
{
	int numFiles = 0;

	QListViewItemIterator it( this );
	while ( it.current() )
	{
		if ( it.current()->childCount() == 0 )
			++numFiles;
		++it;
	}

	return numFiles;
}

int FileListView::selectedFilesCount()
{
	int numFiles = 0;

	QListViewItemIterator it( this, QListViewItemIterator::Selected );
	while ( it.current() )
	{
		++numFiles;
		++it;
	}

	return numFiles;
}

KIO::filesize_t FileListView::totalSize()
{
	KIO::filesize_t size = 0;

	QListViewItemIterator it(this);
	while ( it.current() )
	{
		FileLVI *item = static_cast<FileLVI*>( it.current() );
		size += item->fileSize();
		++it;
	}

	return size;
}

KIO::filesize_t FileListView::selectedSize()
{
	KIO::filesize_t size = 0;

	QListViewItemIterator it( this, QListViewItemIterator::Selected );
	while ( it.current() )
	{
		FileLVI *item = static_cast<FileLVI*>( it.current() );
		size += item->fileSize();
		++it;
	}

	return size;
}

FileLVI* FileListView::findParent( const QString& fullname )
{
	kdDebug( 1601 ) << "findParent: parent for " << fullname << endl;
	QString name = fullname;
	if ( name.endsWith( "/" ) )
		name = name.left( name.length() -1 );
	// Checks if this entry needs a parent
	if ( !name.contains( '/' ) )
		return static_cast< FileLVI* >( 0 );

	// Get a list of ancestors
	QString parentFullname = name.left( name.findRev( '/' ) );
	QStringList ancestorList = QStringList::split( '/', parentFullname );

	// Checks if the listview contains the first item in the list of ancestors
	QListViewItem *item = firstChild();
	while ( item )
	{
		if ( item->text( 0 ) == ancestorList[0] )
			break;
		item = item->nextSibling();
	}

	// If the list view does not contain the item, create it
	if ( !item )
	{
		item = new FileLVI( this );
		item->setText( 0, ancestorList[0] );
		// TODO: Folder Icon

		kdDebug( 1601 ) << "findParent: 	created " << ancestorList[0] << endl;
	}

	// We've already dealt with the first item, remove it
	ancestorList.pop_front();

	while ( ancestorList.count() > 0 )
	{
		QString name = ancestorList[0];

		FileLVI *parent = static_cast< FileLVI*>( item );
		item = parent->firstChild();
		while ( item )
		{
			if ( item->text(0) == name )
				break;
			item = item->nextSibling();
		}

		if ( !item )
		{
			kdDebug( 1601 ) << "findParent: 	created " << ancestorList[0] << endl;
			item = new FileLVI( parent );
			item->setText( 0, name );
			// TODO: Folder icon
		}

		ancestorList.pop_front();
	}

	item->setOpen( true );
	return static_cast< FileLVI* >( item );
}

#include "filelistview.moc"
