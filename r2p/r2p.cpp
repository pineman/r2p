#include "r2p.h"

R2P::R2P(QObject *parent, int localPort)
	: QObject(parent)
	, local(this)
{
	connect(&local, &QTcpServer::acceptError, this, &R2P::socketError);
	connect(&local, &QTcpServer::newConnection, this, &R2P::receiveRequest);

	if (!local.listen(QHostAddress::Any, localPort)) {
		qDebug("Error listening on 0.0.0.0:%d", localPort);
	}
}

R2P::~R2P()
{
}

void R2P::socketError(QAbstractSocket::SocketError error)
{
	qDebug() << "R2P::socketError:" << error;
}

// Receive request
// Messages start with one byte of type Message and end with a newline char.
void R2P::receiveRequest()
{
	QTcpSocket *remote = local.nextPendingConnection();
	connect(remote, static_cast<QAbstractSocketErrorSignal>(&QAbstractSocket::error), this, &R2P::socketError);

	QString *request = new QString;
	connect(remote, &QAbstractSocket::readyRead, [=] ()
	{
		request->append(remote->readAll());

		if (request->contains('\n')) {
			emit gotRequest(remote, request->toUtf8()[0], request->mid(1));
			delete request;
			remote->disconnectFromHost();
			remote->deleteLater();
		}
	});
}

// Send request
void R2P::sendRequest(const QString remoteAddress, const int remotePort,
	const char requestType, const QString payload)
{
	QTcpSocket *remote = new QTcpSocket;
	connect(remote, static_cast<QAbstractSocketErrorSignal>(&QAbstractSocket::error), this, &R2P::socketError);

	remote->connectToHost(remoteAddress, remotePort);

	QByteArray *buf = new QByteArray;
	buf->append(requestType).append(payload).append('\n');
	connect(remote, &QAbstractSocket::connected, [=] ()
	{
		remote->write(*buf);
		remote->flush();
		delete buf;
	});

	QString *reply = new QString;
	connect(remote, &QAbstractSocket::readyRead, [=] ()
	{
		reply->append(remote->readAll());

		if (reply->contains('\n')) {
			emit gotReply(reply->toUtf8()[0], reply->mid(1));
			delete reply;
			remote->disconnectFromHost();
			remote->deleteLater();
		}
	});
}
