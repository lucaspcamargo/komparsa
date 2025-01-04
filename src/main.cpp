#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include <QUrl>
#include <QQuickStyle>
#include <KAboutData>
#include <KLocalizedContext>
#include <KLocalizedString>
#include <KIconTheme>

#include "Engine.h"

int main(int argc, char *argv[])
{
    KIconTheme::initTheme();
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("komparsa");
    QApplication::setOrganizationName(QStringLiteral("camargo.eng.br"));
    QApplication::setOrganizationDomain(QStringLiteral("camargo.eng.br"));
    QApplication::setApplicationName(QStringLiteral("Komparsa"));
    QApplication::setDesktopFileName(QStringLiteral("br.eng.camargo.komparsa"));

    QApplication::setStyle(QStringLiteral("breeze"));
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }
    
    KAboutData aboutData(
        QStringLiteral("Komparsa"),
        i18nc("@title", "Komparsa"),
        QStringLiteral("1.0"),
        i18n("Geminispace browser"),
        KAboutLicense::GPL,
        i18n("(c) 2024"));

    aboutData.addAuthor(
        i18nc("@info:credit", "Lucas Pires Camargo"),
        i18nc("@info:credit", "Developer"),
        QStringLiteral("lucas@camargo.eng.br"),
        QStringLiteral("https://camargo.eng.br"));

    // Set aboutData as information about the app
    KAboutData::setApplicationData(aboutData);
    qmlRegisterSingletonType(
        "br.eng.camargo.komparsa", // How the import statement should look like
        1, 0, // Major and minor versions of the import
        "About", // The name of the QML object
        [](QQmlEngine* engine, QJSEngine *) -> QJSValue {
            return engine->toScriptValue(KAboutData::applicationData());
        }
    );

    qmlRegisterSingletonInstance(
        "br.eng.camargo.komparsa",
        1, 0,
        "Engine",
        new Engine()
    );
    
    QQmlApplicationEngine engine;

    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));
    engine.loadFromModule("br.eng.camargo.komparsa", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
