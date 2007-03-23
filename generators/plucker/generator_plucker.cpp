/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <QtCore/QFile>
#include <QtGui/QAbstractTextDocumentLayout>
#include <QtGui/QPainter>
#include <QtGui/QTextDocument>
#include <QtGui/QTextBlock>

#include <klocale.h>

#include <okular/core/page.h>

#include "generator_plucker.h"

OKULAR_EXPORT_PLUGIN(PluckerGenerator)

static void calculateBoundingRect( QTextDocument *document, int startPosition, int endPosition,
                                   QRectF &rect )
{
    const QTextBlock startBlock = document->findBlock( startPosition );
    const QRectF startBoundingRect = document->documentLayout()->blockBoundingRect( startBlock );

    const QTextBlock endBlock = document->findBlock( endPosition );
    const QRectF endBoundingRect = document->documentLayout()->blockBoundingRect( endBlock );

    QTextLayout *startLayout = startBlock.layout();
    QTextLayout *endLayout = endBlock.layout();

    int startPos = startPosition - startBlock.position();
    int endPos = endPosition - endBlock.position();
    const QTextLine startLine = startLayout->lineForTextPosition( startPos );
    const QTextLine endLine = endLayout->lineForTextPosition( endPos );

    double x = startBoundingRect.x() + startLine.cursorToX( startPos );
    double y = startBoundingRect.y() + startLine.y();
    double r = endBoundingRect.x() + endLine.cursorToX( endPos );
    double b = endBoundingRect.y() + endLine.y() + endLine.height();

    const QSizeF size = document->size();
    rect = QRectF( x / size.width(), y / size.height(),
                   (r - x) / size.width(), (b - y) / size.height() );
}

PluckerGenerator::PluckerGenerator()
    : Generator()
{
    setFeature( Threaded );
}

PluckerGenerator::~PluckerGenerator()
{
    qDeleteAll( mPages );
    mPages.clear();
}

bool PluckerGenerator::loadDocument( const QString & fileName, QVector<Okular::Page*> & pagesVector )
{
    QUnpluck unpluck;

    mLinkAdded.clear();

    qDeleteAll( mPages );
    mPages.clear();

    if ( !unpluck.open( fileName ) )
        return false;

    mPages = unpluck.pages();
    mLinks = unpluck.links();

    const QMap<QString, QString> infos = unpluck.infos();
    QMapIterator<QString, QString> it( infos );
    while ( it.hasNext() ) {
        it.next();
        if ( !it.value().isEmpty() ) {
            if ( it.key() == QLatin1String( "name" ) )
                mDocumentInfo.set( "name", it.value(), i18n( "Name" ) );
            else if ( it.key() == QLatin1String( "title" ) )
                mDocumentInfo.set( "title", it.value(), i18n( "Title" ) );
            else if ( it.key() == QLatin1String( "author" ) )
                mDocumentInfo.set( "author", it.value(), i18n( "Author" ) );
            else if ( it.key() == QLatin1String( "time" ) )
                mDocumentInfo.set( "creationDate", it.value(), i18n( "Created" ) );
        }
    }

    pagesVector.resize( mPages.count() );

    for ( int i = 0; i < mPages.count(); ++i ) {
        QSizeF size = mPages[ i ]->size();
        Okular::Page * page = new Okular::Page( i, size.width(), size.height(), Okular::Rotation0 );
        pagesVector[i] = page;
    }

    return true;
}

bool PluckerGenerator::closeDocument()
{
    return true;
}

const Okular::DocumentInfo* PluckerGenerator::generateDocumentInfo()
{
    return &mDocumentInfo;
}

bool PluckerGenerator::canGeneratePixmap() const
{
    return true;
}

void PluckerGenerator::generatePixmap( Okular::PixmapRequest * request )
{
    const QSizeF size = mPages[ request->pageNumber() ]->size();

    QPixmap *pixmap = new QPixmap( request->width(), request->height() );
    pixmap->fill( Qt::white );

    QPainter p;
    p.begin( pixmap );

    qreal width = request->width();
    qreal height = request->height();

    p.scale( width / (qreal)size.width(), height / (qreal)size.height() );
    mPages[ request->pageNumber() ]->drawContents( &p );
    p.end();

    request->page()->setPixmap( request->id(), pixmap );


    if ( !mLinkAdded.contains( request->pageNumber() ) ) {
        QLinkedList<Okular::ObjectRect*> objects;
        for ( int i = 0; i < mLinks.count(); ++i ) {
            if ( mLinks[ i ].page == request->pageNumber() ) {
                QTextDocument *document = mPages[ request->pageNumber() ];

                QRectF rect;
                calculateBoundingRect( document, mLinks[ i ].start,
                                       mLinks[ i ].end, rect );

                objects.append( new Okular::ObjectRect( rect.left(), rect.top(), rect.right(), rect.bottom(), false, Okular::ObjectRect::Link, mLinks[ i ].link ) );
            }
        }

        if ( !objects.isEmpty() )
            request->page()->setObjectRects( objects );

        mLinkAdded.insert( request->pageNumber() );
    }

    signalPixmapRequestDone( request );
}

Okular::ExportFormat::List PluckerGenerator::exportFormats() const
{
    static Okular::ExportFormat::List formats;
    if ( formats.isEmpty() )
        formats.append( Okular::ExportFormat::plainText() );

    return formats;
}

bool PluckerGenerator::exportTo( const QString &fileName, const Okular::ExportFormat &format )
{
    if ( format.mimeType()->name() == QLatin1String( "text/plain" ) ) {
        QFile file( fileName );
        if ( !file.open( QIODevice::WriteOnly ) )
            return false;

        QTextStream out( &file );
        for ( int i = 0; i < mPages.count(); ++i ) {
            out << mPages[ i ]->toPlainText();
        }

        return true;
    }

    return false;
}

bool PluckerGenerator::print( KPrinter& )
{
/*
    for ( int i = 0; i < mPages.count(); ++i )
      mPages[ i ]->print( &printer );
*/
    return true;
}

#include "generator_plucker.moc"