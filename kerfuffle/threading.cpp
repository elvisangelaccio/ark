/*
 * Copyright (c) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
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

#include "threading.h"
#include <KDebug>

#include <QTimer>

//#define DEBUG_RACECONDITION

namespace Kerfuffle
{
ThreadExecution::ThreadExecution(Kerfuffle::Job *job)
        : m_job(job)
{
}

void ThreadExecution::run()
{
    connect(m_job, SIGNAL(result(KJob*)), this, SLOT(quit()), Qt::DirectConnection);

    QTimer doWorkTimer;
    doWorkTimer.setSingleShot(true);
    connect(&doWorkTimer, SIGNAL(timeout()), m_job, SLOT(doWork()), Qt::DirectConnection);
    doWorkTimer.start(0);

    exec();

#ifdef DEBUG_RACECONDITION
    QThread::sleep(2);
#endif
}
}

#include "threading.moc"