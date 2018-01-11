#include "baseapi.h"

#include <QNetworkReply>
BaseApi::BaseApi(QObject *parent) : QObject(parent)
{
    QObject::connect(&networkAccessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(serviceRequestFinished(QNetworkReply*)));
}

void BaseApi::get(const QString url)
{
    httpRequest.setUrl(QUrl(url));
    networkAccessManager.get(httpRequest);
}

void BaseApi::post(const QString url, const QByteArray &data)
{
    httpRequest.setUrl(QUrl(url));
    networkAccessManager.post(httpRequest, data);
}

void BaseApi::serviceRequestFinished(QNetworkReply *reply)
{
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    qDebug() << "BaseAPI...serviceRequestFinished...statusCode:" << statusCode;

    if(reply->error() == QNetworkReply::NoError) {
        requestFinished(reply, reply->readAll(), statusCode);
    } else {
        requestFinished(reply, "", statusCode);
    }
    reply->deleteLater();
}
