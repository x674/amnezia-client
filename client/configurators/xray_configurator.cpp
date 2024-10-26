#include "xray_configurator.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <qlogging.h>

#include "containers/containers_defs.h"
#include "core/controllers/serverController.h"
#include "core/scripts_registry.h"

XrayConfigurator::XrayConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent)
    : ConfiguratorBase(settings, serverController, parent)
{
}

QString XrayConfigurator::prepareServerConfig(const ServerCredentials &credentials, DockerContainer container,
                                               const QJsonObject &containerConfig, ErrorCode &errorCode)
{
    // Generate new UUID for client
    QString clientId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // Create configuration for new client
    QString clientConfig = QString(R"(
                {
                    "id": "%1",
                    "flow": "xtls-rprx-vision"
                })")
            .arg(clientId);

    // Get current server config
    QString currentConfig = m_serverController->getTextFileFromContainer(
        container, credentials, amnezia::protocols::xray::serverConfigPath, errorCode);
    
    if (errorCode != ErrorCode::NoError) {
        return "";
    }

    // Parse current config as JSON
    QJsonDocument doc = QJsonDocument::fromJson(currentConfig.toUtf8());
    if (doc.isNull()) {
        errorCode = ErrorCode::InternalError;
        return "";
    }

    QJsonObject serverConfig = doc.object();
    
    if (!serverConfig.contains("inbounds") || serverConfig["inbounds"].toArray().isEmpty()) {
        return "";
    }
    
    QJsonArray inbounds = serverConfig["inbounds"].toArray();
    QJsonObject inbound = inbounds[0].toObject();
    QJsonObject settings = inbound["settings"].toObject();
    QJsonArray clients = settings["clients"].toArray();
    
    clients.append(QJsonDocument::fromJson(clientConfig.toUtf8()).object());
    
    // Update config
    settings["clients"] = clients;
    inbound["settings"] = settings;
    inbounds[0] = inbound;
    serverConfig["inbounds"] = inbounds;
    
    // Save updated config to server
    QString updatedConfig = QJsonDocument(serverConfig).toJson();
    errorCode = m_serverController->uploadTextFileToContainer(
        container, 
        credentials, 
        updatedConfig,
        amnezia::protocols::xray::serverConfigPath,
        libssh::ScpOverwriteMode::ScpOverwriteExisting
    );
    if (errorCode != ErrorCode::NoError) {
        return "";
    }

    // Restart container
    QString restartScript = QString("docker restart $CONTAINER_NAME");
    errorCode = m_serverController->runScript(
        credentials, 
        m_serverController->replaceVars(restartScript, m_serverController->genVarsForScript(credentials, container))
    );

    if (errorCode != ErrorCode::NoError) {
        return "";
    }

    return clientId;
}

QString XrayConfigurator::createConfig(const ServerCredentials &credentials, DockerContainer container,
                                       const QJsonObject &containerConfig, ErrorCode &errorCode)
{
    // Get client ID from prepareServerConfig
    QString xrayClientId = prepareServerConfig(credentials, container, containerConfig, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return "";
    }

    QString config = m_serverController->replaceVars(amnezia::scriptData(ProtocolScriptType::xray_template, container),
                                                     m_serverController->genVarsForScript(credentials, container, containerConfig));

    QString xrayPublicKey =
            m_serverController->getTextFileFromContainer(container, credentials, amnezia::protocols::xray::PublicKeyPath, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return "";
    }
    xrayPublicKey.replace("\n", "");
    
    QString xrayShortId =
            m_serverController->getTextFileFromContainer(container, credentials, amnezia::protocols::xray::shortidPath, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return "";
    }
    xrayShortId.replace("\n", "");

    config.replace("$XRAY_CLIENT_ID", xrayClientId);
    config.replace("$XRAY_PUBLIC_KEY", xrayPublicKey);
    config.replace("$XRAY_SHORT_ID", xrayShortId);

    qDebug() << "===>> xrayClientId: " << xrayClientId;
    return config;
}
