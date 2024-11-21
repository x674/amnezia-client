import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    Connections {
        target: ImportController

        function onQrDecodingFinished() {
            if (Qt.platform.os === "ios") {
                PageController.closePage()
            }
            PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
        }
    }

    QtObject {
        id: amneziaVpn

        property string title: qsTr("VPN by Amnezia")
        property string description: qsTr("Connect to classic paid and free VPN services from Amnezia")
        property string imageSource: "qrc:/images/controls/amnezia.svg"
        property bool isVisible: true
        property var handler: function() {
            PageController.showBusyIndicator(true)
            var result = InstallController.fillAvailableServices()
            PageController.showBusyIndicator(false)
            if (result) {
                PageController.goToPage(PageEnum.PageSetupWizardApiServicesList)
            }
        }
    }

    QtObject {
        id: selfHostVpn

        property string title: qsTr("Self-hosted VPN")
        property string description: qsTr("Configure Amnezia VPN on your own server")
        property string imageSource: "qrc:/images/controls/server.svg"
        property bool isVisible: true
        property var handler: function() {
            PageController.goToPage(PageEnum.PageSetupWizardCredentials)
        }
    }

    QtObject {
        id: backupRestore

        property string title: qsTr("Restore from backup")
        property string description: qsTr("")
        property string imageSource: "qrc:/images/controls/archive-restore.svg"
        property bool isVisible: true
        property var handler: function() {
            var filePath = SystemController.getFileName(qsTr("Open backup file"),
                                                        qsTr("Backup files (*.backup)"))
            if (filePath !== "") {
                PageController.showBusyIndicator(true)
                SettingsController.restoreAppConfig(filePath)
                PageController.showBusyIndicator(false)
            }
        }
    }

    QtObject {
        id: fileOpen

        property string title: qsTr("File with connection settings")
        property string description: qsTr("")
        property string imageSource: "qrc:/images/controls/folder-search-2.svg"
        property bool isVisible: true
        property var handler: function() {
            var nameFilter = !ServersModel.getServersCount() ? "Config or backup files (*.vpn *.ovpn *.conf *.json *.backup)" :
                                                               "Config files (*.vpn *.ovpn *.conf *.json)"
            var fileName = SystemController.getFileName(qsTr("Open config file"), nameFilter)
            if (fileName !== "") {
                if (ImportController.extractConfigFromFile(fileName)) {
                    PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
                }
            }
        }
    }

    QtObject {
        id: qrScan

        property string title: qsTr("QR code")
        property string description: qsTr("")
        property string imageSource: "qrc:/images/controls/scan-line.svg"
        property bool isVisible: SettingsController.isCameraPresent()
        property var handler: function() {
            ImportController.startDecodingQr()
            if (Qt.platform.os === "ios") {
                PageController.goToPage(PageEnum.PageSetupWizardQrReader)
            }
        }
    }

    QtObject {
        id: siteLink

        property string title: qsTr("I have nothing")
        property string description: qsTr("")
        property string imageSource: "qrc:/images/controls/help-circle.svg"
        property bool isVisible: true
        property var handler: function() {
            Qt.openUrlExternally(LanguageModel.getCurrentSiteUrl())
        }
    }

    property list<QtObject> variants: [
        amneziaVpn,
        selfHostVpn,
        backupRestore,
        fileOpen,
        qrScan,
        siteLink
    ]

    ListView {
        id: listView

        anchors.fill: parent

        property bool isFocusable: true

        Keys.onTabPressed: {
            FocusController.nextKeyTabItem()
        }

        Keys.onBacktabPressed: {
            FocusController.previousKeyTabItem()
        }

        Keys.onUpPressed: {
            FocusController.nextKeyUpItem()
        }

        Keys.onDownPressed: {
            FocusController.nextKeyDownItem()
        }

        Keys.onLeftPressed: {
            FocusController.nextKeyLeftItem()
        }

        Keys.onRightPressed: {
            FocusController.nextKeyRightItem()
        }

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        model: variants

        clip: true

        header: ColumnLayout {
            width: listView.width

            HeaderType {
                id: moreButton

                property bool isVisible: SettingsController.getInstallationUuid() !== "" || PageController.isStartPageVisible()
                
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Connection")

                actionButtonImage: isVisible ? "qrc:/images/controls/more-vertical.svg" : ""
                actionButtonFunction: function() {
                    moreActionsDrawer.openTriggered()
                }

                DrawerType2 {
                    id: moreActionsDrawer

                    parent: root

                    anchors.fill: parent
                    expandedHeight: root.height * 0.5

                    expandedStateContent: ColumnLayout {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        spacing: 0

                        HeaderType {
                            Layout.fillWidth: true
                            Layout.topMargin: 32
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16

                            headerText: qsTr("Settings")
                        }

                        SwitcherType {
                            id: switcher
                            Layout.fillWidth: true
                            Layout.topMargin: 16
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16

                            text: qsTr("Enable logs")

                            visible: PageController.isStartPageVisible()
                            checked: SettingsController.isLoggingEnabled
                            onCheckedChanged: {
                                if (checked !== SettingsController.isLoggingEnabled) {
                                    SettingsController.isLoggingEnabled = checked
                                }
                            }
                        }

                        LabelWithButtonType {
                            id: supportUuid
                            Layout.fillWidth: true
                            Layout.topMargin: 16

                            text: qsTr("Support tag")
                            descriptionText: SettingsController.getInstallationUuid()

                            descriptionOnTop: true

                            rightImageSource: "qrc:/images/controls/copy.svg"
                            rightImageColor: AmneziaStyle.color.paleGray

                            visible: SettingsController.getInstallationUuid() !== ""
                            clickedFunction: function() {
                                GC.copyToClipBoard(descriptionText)
                                PageController.showNotificationMessage(qsTr("Copied"))
                                if (!GC.isMobile()) {
                                    this.rightButton.forceActiveFocus()
                                }
                            }
                        }
                    }
                }
            }

            ParagraphTextType {
                objectName: "insertKeyLabel"

                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 24

                text: qsTr("Insert the key, add a configuration file or scan the QR-code")
            }

            TextFieldWithHeaderType {
                id: textKey

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                headerText: qsTr("Insert key")
                buttonText: qsTr("Insert")

                clickedFunc: function() {
                    textField.text = ""
                    textField.paste()
                }
            }

            BasicButtonType {
                id: continueButton

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                visible: textKey.textFieldText !== ""

                text: qsTr("Continue")

                clickedFunc: function() {
                    if (ImportController.extractConfigFromData(textKey.textFieldText)) {
                        PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
                    }
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 24

                color: AmneziaStyle.color.charcoalGray
                text: qsTr("Other connection options")
            }
        }

        delegate: ColumnLayout {
            width: listView.width

            CardWithIconsType {
                id: entry

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 16

                visible: isVisible

                headerText: title
                bodyText: description

                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                leftImageSource: imageSource

                onClicked: { handler() }
            }
        }
    }
}
