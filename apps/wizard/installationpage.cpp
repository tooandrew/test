#include "installationpage.hpp"

#include <QDebug>
#include <QTextCodec>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>

#include "mainwizard.hpp"
#include "inisettings.hpp"

Wizard::InstallationPage::InstallationPage(MainWizard *wizard) :
    QWizardPage(wizard),
    mWizard(wizard)
{
    setupUi(this);

    mFinished = false;
}

void Wizard::InstallationPage::initializePage()
{
    QString path(field("installation.path").toString());
    QStringList components(field("installation.components").toStringList());

    logTextEdit->append(QString("Installing to %1").arg(path));
    logTextEdit->append(QString("Installing %1.").arg(components.join(", ")));

    installProgressBar->setMinimum(0);

    // Set the progressbar maximum to a multiple of 100
    // That way installing all three components would yield 300%
    // When one component is done the bar will be filled by 33%

    if (field("installation.new").toBool() == true)
    {
        installProgressBar->setMaximum((components.count() * 100));
    }
    else
    {
        if (components.contains(QLatin1String("Tribunal"))
                && mWizard->mInstallations[path]->hasTribunal == false)
            installProgressBar->setMaximum(100);

        if (components.contains(QLatin1String("Bloodmoon"))
                && mWizard->mInstallations[path]->hasBloodmoon == false)
            installProgressBar->setMaximum(installProgressBar->maximum() + 100);
    }

    startInstallation();
}

void Wizard::InstallationPage::startInstallation()
{
    QStringList components(field("installation.components").toStringList());
    QString path(field("installation.path").toString());

    QThread *thread = new QThread();
    mUnshield = new UnshieldWorker();
    mUnshield->moveToThread(thread);

    qRegisterMetaType<Wizard::Component>("Wizard::Component");

    connect(thread, SIGNAL(started()),
            mUnshield, SLOT(extract()));

    connect(mUnshield, SIGNAL(finished()),
            thread, SLOT(quit()));

    connect(mUnshield, SIGNAL(finished()),
            mUnshield, SLOT(deleteLater()));

    connect(mUnshield, SIGNAL(finished()),
            thread, SLOT(deleteLater()));

    connect(mUnshield, SIGNAL(finished()),
            this, SLOT(installationFinished()), Qt::QueuedConnection);

    connect(mUnshield, SIGNAL(error(QString)),
            this, SLOT(installationError(QString)), Qt::QueuedConnection);

    connect(mUnshield, SIGNAL(textChanged(QString)),
            installProgressLabel, SLOT(setText(QString)), Qt::QueuedConnection);

    connect(mUnshield, SIGNAL(textChanged(QString)),
            logTextEdit, SLOT(append(QString)),  Qt::QueuedConnection);

    connect(mUnshield, SIGNAL(progressChanged(int)),
            installProgressBar, SLOT(setValue(int)),  Qt::QueuedConnection);

    connect(mUnshield, SIGNAL(requestFileDialog(Wizard::Component)),
            this, SLOT(showFileDialog(Wizard::Component)), Qt::QueuedConnection);

    if (field("installation.new").toBool() == true)
    {
        // Always install Morrowind
        mUnshield->setInstallComponent(Wizard::Component_Morrowind, true);

        if (components.contains(QLatin1String("Tribunal")))
            mUnshield->setInstallComponent(Wizard::Component_Tribunal, true);

        if (components.contains(QLatin1String("Bloodmoon")))
            mUnshield->setInstallComponent(Wizard::Component_Bloodmoon, true);
    } else {
        // Morrowind should already be installed
        mUnshield->setInstallComponent(Wizard::Component_Morrowind, false);

        if (components.contains(QLatin1String("Tribunal"))
                && !mWizard->mInstallations[path]->hasTribunal)
            mUnshield->setInstallComponent(Wizard::Component_Tribunal, true);

        if (components.contains(QLatin1String("Bloodmoon"))
                && !mWizard->mInstallations[path]->hasBloodmoon)
            mUnshield->setInstallComponent(Wizard::Component_Bloodmoon, true);

        // Set the location of the Morrowind.ini to update
        mUnshield->setIniPath(mWizard->mInstallations[path]->iniPath);
    }

    // Set the installation target path
    mUnshield->setPath(path);

    // Set the right codec to use for Morrowind.ini
    QString language(field("installation.language").toString());

    if (language == QLatin1String("Polish")) {
        mUnshield->setIniCodec(QTextCodec::codecForName("windows-1250"));
    }
    else if (language == QLatin1String("Russian")) {
        mUnshield->setIniCodec(QTextCodec::codecForName("windows-1251"));
    }
    else {
        mUnshield->setIniCodec(QTextCodec::codecForName("windows-1252"));
    }

    thread->start();
}

void Wizard::InstallationPage::showFileDialog(Wizard::Component component)
{
    QString fileName = QFileDialog::getOpenFileName(
                    this,
                    tr("Select installation file"),
                    QDir::rootPath(),
                    tr("InstallShield header files (*.hdr)"));

    if (fileName.isEmpty()) {
        qDebug() << "Cancel was clicked!";
        return;
    }

    QFileInfo info(fileName);
    mUnshield->setComponentPath(component, info.absolutePath());
}

void Wizard::InstallationPage::installationFinished()
{
    qDebug() << "finished!";

    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Installation finished"));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setText(tr("Installation completed sucessfully!"));

    msgBox.exec();

    mFinished = true;
    emit completeChanged();

}

void Wizard::InstallationPage::installationError(const QString &text)
{
    qDebug() << "error: " << text;
}

bool Wizard::InstallationPage::isComplete() const
{
    return mFinished;
}

int Wizard::InstallationPage::nextId() const
{
    return MainWizard::Page_Import;
}
