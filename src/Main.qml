import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import br.eng.camargo.komparsa as Komparsa

Kirigami.ApplicationWindow {
    // Unique identifier to reference this object
    id: root

    width: 1000
    height: 750

    title: i18nc("@title:window", "Komparsa")
    
    globalDrawer: Kirigami.GlobalDrawer {
        title: root.title
        titleIcon: "internet-web-browser"
        modal: false
        collapsible: true
        collapsed: true
        showHeaderWhenCollapsed: false

        actions: [
            Kirigami.Action {
                text: i18n("Library")
                icon.name: "folder-library-symbolic"
                onTriggered: { let on = checked; pageStack.clear(); if(!on) pageStack.push(libraryPage); pageStack.push(viewPage); }
                checked: pageStack.get(0) === libraryPage;
            },
            Kirigami.Action {
                text: i18n("Identities")
                icon.name: "system-users-symbolic"
                onTriggered: { let on = checked; pageStack.clear(); if (!on) pageStack.push(identitiesPage); pageStack.push(viewPage); }
                checked: pageStack.get(0) === identitiesPage;
            },
            Kirigami.Action {
                text: i18n("About")
                icon.name: "help-about"
                onTriggered: { let on = checked; pageStack.clear(); if (!on) pageStack.push(aboutPage); pageStack.push(viewPage); }
                checked: pageStack.get(0) === aboutPage;
            }
        ]
    }

    pageStack.initialPage: [libraryPage, viewPage]


    Kirigami.Page {
        title: "Library"
        id: libraryPage

        ListView {
            anchors.fill: parent
            model: libraryModel
            delegate: libraryDelegate
        }
        ListModel {
            id: libraryModel
            ListElement { displayName: "localhost"; url: "gemini://localhost/" }
            ListElement { displayName: "camargo.eng.br"; doctitle: "Welcome!";  url: "gemini://camargo.eng.br/" }
            ListElement { displayName: "geminiprotocol.net"; doctitle: "Gemini Project"; url: "gemini://geminiprotocol.net/" }
            ListElement { displayName: "gemini.circumlunar.space/capcom/"; url: "gemini://gemini.circumlunar.space/capcom/" }
            ListElement { displayName: "medusae.space"; url: "gemini://medusae.space/" }
        }
        Component {
            id: libraryDelegate
            Controls.ItemDelegate {
                width: ListView.view.width
                text: model.displayName + (model.doctitle? ` - ${model.doctitle}`:"")
                icon.name: "globe-symbolic"
                onClicked: { viewPage.currentURL = model.url; }
            }
        }
    }

    Kirigami.Page {
        id: identitiesPage
        title: "Identities"
    }

    Kirigami.AboutPage {
        id: aboutPage
        aboutData: Komparsa.About
    }


    Kirigami.ScrollablePage {
        // main view
        id: viewPage
        
        property string currentURL: "gemini://geminiprotocol.net/docs/cheatsheet.gmi"
        onCurrentURLChanged: Komparsa.Engine.retrieve(currentURL);
        
        Kirigami.Theme.colorSet: Kirigami.Theme.View
        
        title: new URL(currentURL).host

        header: Kirigami.AbstractApplicationHeader { contentItem: RowLayout { 
            anchors.left: parent.left
            anchors.right: parent.right

            Kirigami.ActionTextField {
                id: urlField
                Layout.fillWidth: true
                
                text: viewPage.currentURL
                placeholderText: "Type a gemini:// location..."

                onAccepted: {
                    viewPage.currentURL = urlField.text;
                }
                
                leftActions: [Kirigami.Action {
                    id: cryptoAction
                    icon.name: "channel-secure-symbolic"
                    icon.color: Theme.View.positiveTextColor
                }]
            }
        } }
        
        actions: [
            Kirigami.Action {
                id: backAction
                icon.name: "draw-arrow-back"
                text: i18nc("@action:button", "Go Back")
                onTriggered: {  }
                displayHint: Kirigami.DisplayHint.IconOnly
            },
            Kirigami.Action {
                id: forwardAction
                icon.name: "draw-arrow-forward"
                text: i18nc("@action:button", "Go Forward")
                onTriggered: {  }
                displayHint: Kirigami.DisplayHint.IconOnly
            },
            Kirigami.Action {
                id: goHomeAction
                icon.name: "go-home-symbolic"
                text: i18nc("@action:button", "Go Home")
                onTriggered: { viewPage.currentURL = "gemini://localhost/"; }
                displayHint: Kirigami.DisplayHint.IconOnly
            },
            Kirigami.Action {
                id: sourceAction
                icon.name: "format-text-code-symbolic"
                text: i18nc("@action:button", "View Source")
                onTriggered: { }
                displayHint: Kirigami.DisplayHint.IconOnly
            },
            Kirigami.Action {
                id: downloadAction
                icon.name: "download-symbolic"
                text: i18nc("@action:button", "Download")
                onTriggered: {  }
                displayHint: Kirigami.DisplayHint.IconOnly
            }
        ]
        
        Controls.TextArea {
            id: renderArea
            width: parent.width
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            readOnly: true
            background: null
            selectByMouse: true

            HoverHandler {
                enabled: parent.hoveredLink
                cursorShape: Qt.PointingHandCursor
            }
            onLinkActivated: (url) => {
                let curr = new URL(viewPage.currentURL);
                try
                {
                    const newUrl = new URL(url, viewPage.currentURL);
                    viewPage.currentURL = newUrl;
                }
                catch (error)
                {
                    console.log("invalid URL:", error);
                }
            }
            
            text: ""
            
            Connections {
                target: Komparsa.Engine
                function onRetrievalDone( ok, url, document ) {
                    renderArea.text = document;
                }
                function onCertificateRequested( url, deets ) {
                    certificateDialog.requester = url;
                    certificateDialog.details = deets;
                    certificateDialog.fillSubtitle();
                    certificateDialog.open();
                }
            }
        }


        
        Kirigami.PromptDialog {
            id: certificateDialog
            property string requester: ""
            property string details: ""
            title: i18n("Certificate Requested")
            standardButtons: Kirigami.Dialog.Yes | Kirigami.Dialog.Cancel

            function fillSubtitle()
            {
                if(details != "")
                    subtitle = i18n("The server for '%1' requires a certificate.\nThe server says: '%2'.\nGenerate a temporary one and retry?").arg(requester).arg(details);
                else
                subtitle = i18n("The server for '%1' requires a certificate.\nGenerate a temporary one and retry?").arg(requester);
            }
            onAccepted: {
                Komparsa.Engine.loadTempCertificate(requester);
                Komparsa.Engine.retrieve(requester);
            }
        }
    }
    
    Component.onCompleted: {
        console.log("Window seems ready.");
        Komparsa.Engine.retrieve(viewPage.currentURL);
    }
    
}
