/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Bertrand Martel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
/**
    HttpServer.cpp

    Http server main process class

    manage incoming connections
    manage socket encryption for SSL socket
    manage process of incoming data from client socket

    @author Bertrand Martel
    @version 1.0
*/
#include "httpserver.h"
#include "iostream"
#include "protocol/inter/http/httpconsumer.h"
#include "string"
#include "clientSocket.h"
#include "utils/stringutils.h"
#include "QtGlobal"

using namespace std;

/**
 * @brief HttpServer::socketClientList
 *      static list featuring all socket client connected to server
 */
std::map<QSslSocket*,ClientSocket > HttpServer::socketClientList;

/**
 * @brief HttpServer::HttpServer
 *      construct for HTTP server init new connection signal and set consumer
 *
 * @param parent
 */
HttpServer::HttpServer(QObject* parent): QTcpServer(parent)
{
    connect(this, SIGNAL(newConnection()), this, SLOT(handleNewConnection()));
    consumer=new httpconsumer;
    debug=true;

    //secure socket is disable for default config
    ssl=false;
}

/**
 * @brief HttpServer::setSSL
 *      set HTTP server to secured HTTP server
 * @param use_ssl
 */
void HttpServer::setSSL(bool use_ssl)
{
    ssl=use_ssl;
}

/**
 * @brief HttpServer::setPublicCert
 *      set public server cert
 * @param cert
 *      public certificate
 */
void HttpServer::setPublicCert(QSslCertificate cert)
{
    localCertificate=cert;
}

/**
 * @brief HttpServer::setCaCert
 *      set certification authoritycert
 * @param cert
 *      certification authority cert
 */
void HttpServer::setCaCert(QList< QSslCertificate > cert)
{
    caCertificate=cert;
}

/**
 * @brief HttpServer::setPrivateCert
 *      set private certificate
 * @param cert
 *      private certificate
 */
void HttpServer::setPrivateCert(QSslKey key)
{
    keyCertificate=key;
}

/**
 * @brief HttpServer::handleNewConnection
 *      a new connection has come to server
 */
void HttpServer::handleNewConnection()
{
    QSslSocket *clientSocket;

    clientSocket = qobject_cast<QSslSocket *>(sender());

    if (debug)
        qDebug() << "New connection detected..." << endl;

    clientSocket = this->nextPendingConnection();

    if (!clientSocket) {
        qWarning("ERROR => not enough memory to create new QSslSocket");
        return;
    }

    // connect signals to slot
    connectSocketSignals(clientSocket);

    // only for ssl encryption
    if (ssl)
        startServerEncryption((QSslSocket *) clientSocket);
}

void HttpServer::incomingConnection(int socketDescriptor)
{
    if (debug)
        qDebug("incomingConnection(%d)", (int)socketDescriptor);

    QSslSocket *newSocket = new QSslSocket(this);

    if(!newSocket->setSocketDescriptor(socketDescriptor))
        return;

    queue.enqueue(newSocket);

}

QSslSocket* HttpServer::nextPendingConnection()
{
    if (debug)
        cout << "pending connection" << endl;

    if(queue.isEmpty())
        return 0;
    else
        return queue.dequeue();
}

/**
 * @brief HttpServer::connectSocketSignals
 *      connect signals to slots : we take the maximum of slots here to manage errors quickly
 * @param clientSocket
 *      client socket incoming
 */
void HttpServer::connectSocketSignals (QSslSocket* clientSocket)
{
    //slots for all socket types
    connect(clientSocket, SIGNAL(readyRead())                        ,this, SLOT(incomingData()));
    connect(clientSocket, SIGNAL(connected())                        ,this, SLOT(slot_connected()));
    connect(clientSocket, SIGNAL(disconnected())                     ,this, SLOT(slot_disconnected()));
    connect(clientSocket, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(slot_error(QAbstractSocket::SocketError)));
    connect(clientSocket, SIGNAL(hostFound())                        ,this, SLOT(slot_hostFound()));
    connect(clientSocket, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)),this, SLOT(slot_proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)));
    connect(clientSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),this, SLOT(slot_stateChanged(QAbstractSocket::SocketState)));
    if (ssl)
    {
        //slots for ssl cert handshake process and error monitoring
        connect(clientSocket, SIGNAL(encrypted())                        ,this, SLOT(slot_encrypted()));
        connect(clientSocket, SIGNAL(encryptedBytesWritten(qint64))      ,this, SLOT(slot_encryptedBytesWritten(qint64)));
        connect(clientSocket, SIGNAL(modeChanged(QSslSocket::SslMode))   ,this, SLOT(slot_modeChanged(QSslSocket::SslMode)));
        connect(clientSocket, SIGNAL(peerVerifyError(const QSslError &)) ,this, SLOT(slot_peerVerifyError (const QSslError &)));
        connect(clientSocket, SIGNAL(sslErrors(const QList<QSslError> &)),this, SLOT(slot_sslErrors(const QList<QSslError> &)));
    }
}

/**
 * @brief HttpServer::startServerEncryption
 *      add respective certificates for SSL encryption
 *
 * @param clientSocket
 *      ssl client socket
 */
void HttpServer::startServerEncryption (QSslSocket* clientSocket)
{
    if (keyCertificate.isNull() || localCertificate.isNull() || caCertificate.isEmpty())
    {
        cout << "Error invalid certificates" << endl;
        clientSocket->close();
        return;
    }

    if (debug)
        cout << "server encryption" << endl;

    if (debug)
        qDebug("setting private key...");

    clientSocket->setPrivateKey(keyCertificate);

    if (debug)
        qDebug("setting local certificate...");

    clientSocket->setLocalCertificate(localCertificate);

    if (debug)
        qDebug("adding ca certificates...");

    clientSocket->addCaCertificates(caCertificate);

    clientSocket->setProtocol(QSsl::AnyProtocol);

    if (debug)
        qDebug("starting server encryption...");

    clientSocket->startServerEncryption();

}

/**
 * @brief HttpServer::slot_disconnected
 *      slot used when one client socket disconnect form HTTP server
 */
void HttpServer::slot_disconnected()
{
    if (debug)
        qDebug() <<  "slot_disconnected() : Socket disconnected..." << endl;

     QSslSocket *client = qobject_cast<QSslSocket *>(sender());

    if (!client)
        return;

    //remove client socket from list
    socketClientList.erase(client);

    //safe delete
    client->deleteLater();
}

void HttpServer::ready()
{
}

void HttpServer::slot_encryptedBytesWritten (qint64 written)
{
    if (debug)
        qDebug("QMyServer::slot_encryptedBytesWritten(%ld)", (long) written);
}

void HttpServer::slot_modeChanged (QSslSocket::SslMode mode)
{
    if (debug)
        qDebug("QMyServer::slot_modeChanged(%d)", mode);
}

void HttpServer::slot_peerVerifyError (const QSslError &)
{
    if (debug)
        qDebug("QMyServer::slot_peerVerifyError");
}

void HttpServer::slot_sslErrors (const QList<QSslError> &)
{
    if (debug)
        qDebug("QMyServer::slot_sslErrors");
}

void HttpServer::slot_encrypted()
{
    if (debug)
        qDebug("QMyServer::slot_encrypted");
}
void HttpServer::slot_connected ()
{
    if (debug)
        qDebug("QMyServer::slot_connected");
}

void HttpServer::slot_error (QAbstractSocket::SocketError err)
{
    QSslSocket *client = qobject_cast<QSslSocket *>(sender());

    if (debug)
        cout << "received error !" << endl;

    if (debug)
        qDebug() << __PRETTY_FUNCTION__ << err << client->errorString();
}

void HttpServer::slot_hostFound ()
{
    if (debug)
        qDebug("QMyServer::slot_hostFound");
}

void HttpServer::slot_proxyAuthenticationRequired (const QNetworkProxy &, QAuthenticator *)
{
    if (debug)
        qDebug("QMyServer::slot_proxyAuthenticationRequired");
}

void HttpServer::slot_stateChanged (QAbstractSocket::SocketState state)
{
    if (debug)
        qDebug() << "QMyServer::slot_stateChanged(" << state << ")";
}

/**
 * @brief HttpServer::~HttpServer
 *      desctruct => delete pointers
 */
HttpServer::~HttpServer()
{
  delete consumer;
  consumer=0;
}

/**
 * @brief HttpServer::incomingData
 *      that slot is for data coming from client socket
 */
void HttpServer::incomingData()
{
    QSslSocket *clientSocket = qobject_cast<QSslSocket *>(sender());

    //we manage socket client object from static list (thats where we store client)
    ClientSocket obj =HttpServer::socketClientList[clientSocket];

    obj.setSocketClient(clientSocket);

    QByteArray data = clientSocket->readAll();

    //http streaming data is decoded here
    decoder.httpdecode(consumer,&data);

    if (consumer->getHttpFrameList().size()>0)
    {
        int count = consumer->getHttpFrameList().size()-1;

        //iterate through all http streaming frames
        while (containsHttpProcessedFrames(consumer->getHttpFrameList()))
        {
            //take into account only http frames that have been processed successfully
            if (consumer->getHttpFrameList().at(count)->isFinishedProcessing())
            {
                if (strcmp(consumer->getHttpFrameList().at(count)->getMethod().data(),"")==0)
                {
                    for (unsigned int i = 0; i  < this->clientEventListenerList.size();i++)
                    {
                        this->clientEventListenerList.at(i)->onHttpResponseReceived(obj,consumer->getHttpFrameList().at(count));
                    }
                }
                else
                {
                    for (unsigned int i = 0; i  < this->clientEventListenerList.size();i++)
                    {
                        this->clientEventListenerList.at(i)->onHttpRequestReceived(obj,consumer->getHttpFrameList().at(count));
                    }
                }

                //last element of http frame must be removed to avoid to be popped next time we process frames
                std::vector<Ihttpframe*> frameList = consumer->getHttpFrameList();
                frameList.pop_back();
                consumer->setHttpFrameList(&frameList);

            }
            else
            {
                cout << "Current HTTP frame has not been processed correctly." << endl;
            }
            count--;
        }

        cout << "remaining => " << consumer->getHttpFrameList().size() << endl;
    }

    //client socket must be closed
    closeClientSocket(clientSocket);

    // we store pointer to client socket to be reused at any time
    socketClientList[clientSocket]= obj;
}

/**
 * @brief HttpServer::containsHttpProcessedFrames
 *      check if http frame list buffer already contains finished http processed frame or no
 * @param frameList
 *      list of http frames
 * @return
 */
bool HttpServer::containsHttpProcessedFrames(std::vector<Ihttpframe*> frameList)
{
    for (int i = 0; i < frameList.size();i++)
    {
        if (frameList.at(i)->isFinishedProcessing())
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief HttpServer::addClientEventListener
 *      add a client event listener to list
 * @param clientListener
 *      client listener
 */
void HttpServer::addClientEventListener(IClientEventListener *clientListener)
{
    this->clientEventListenerList.push_back(clientListener);
}

/**
 * @brief HttpServer::closeClientSocket
 *      close client socket function
 * @param socket
 *      client socket
 */
void HttpServer::closeClientSocket(QSslSocket* socket)
{
    cout << "closing socket..." << endl;

    if (socket->isOpen())
        socket->close();

    //manage unconnected state
    if (socket->state() == QSslSocket::UnconnectedState) {
         delete socket;
         cout << "Connection closed" << endl;
    }
}
