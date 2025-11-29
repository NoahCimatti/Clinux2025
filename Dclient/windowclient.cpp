#include "windowclient.h"
#include "ui_windowclient.h"
#include <QMessageBox>
#include "Modif/dialogmodification.h"
#include <unistd.h>
#include "protocole.h"

extern WindowClient *w;

#include "protocole.h"

int idQ, idShm;
#define TIME_OUT 120
int timeOut = TIME_OUT;

void handlerSIGUSR1(int sig);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
WindowClient::WindowClient(QWidget *parent):QMainWindow(parent),ui(new Ui::WindowClient)
{
    ui->setupUi(this);
    ::close(2);
    logoutOK();

    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la file de messages\n",getpid());

    // Recuperation de l'identifiant de la mémoire partagée
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la mémoire partagée\n",getpid());

    // Attachement à la mémoire partagée

    // Armement des signaux
    // SIGUSR1 :
    struct sigaction sa_sigusr1;
    sa_sigusr1.sa_handler = handlerSIGUSR1;
    sigemptyset(&sa_sigusr1.sa_mask);
    sa_sigusr1.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa_sigusr1, NULL) == -1) 
    { 
      perror("sigaction de SIGUSR1"); 
      exit(1); 
    }

    // SIGUSR2 :
    struct sigaction sa_sigusr2;
    sa_sigusr2.sa_handler = handlerSIGUSR2;
    sigemptyset(&sa_sigusr2.sa_mask);
    sa_sigusr2.sa_flags = 0;
    if (sigaction(SIGUSR2, &sa_sigusr2, NULL) == -1) 
    { 
      perror("sigaction de SIGUSR2"); 
      exit(1); 
    }

    // Envoi d'une requete de connexion au serveur
    fprintf(stderr,"(CLIENT %d) Envoi requete CONNEXION au serveur\n",getpid());
    MESSAGE connectCLIENT;
    connectCLIENT.type = 1;
    connectCLIENT.expediteur = getpid();
    connectCLIENT.requete = CONNECT;

    if (msgsnd(idQ,&connectCLIENT,sizeof(MESSAGE)-sizeof(long),0) == -1)
    {
        perror("(CLIENT) Erreur envoi requete CONNECT");
        exit(1);
    }
    fprintf(stderr,"Processus %d a envoye un message d'identification de type CONNECT au serveur avec succes\n",getpid());

}

WindowClient::~WindowClient()
{
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles : ne pas modifier /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setNom(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditNom->clear();
    return;
  }
  ui->lineEditNom->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getNom()
{
  strcpy(connectes[0],ui->lineEditNom->text().toStdString().c_str());
  return connectes[0];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setMotDePasse(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditMotDePasse->clear();
    return;
  }
  ui->lineEditMotDePasse->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getMotDePasse()
{
  strcpy(motDePasse,ui->lineEditMotDePasse->text().toStdString().c_str());
  return motDePasse;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowClient::isNouveauChecked()
{
  if (ui->checkBoxNouveau->isChecked()) return 1;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setPublicite(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditPublicite->clear();
    return;
  }
  ui->lineEditPublicite->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setTimeOut(int nb)
{
  ui->lcdNumberTimeOut->display(nb);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setAEnvoyer(const char* Text)
{
  //fprintf(stderr,"---%s---\n",Text);
  if (strlen(Text) == 0 )
  {
    ui->lineEditAEnvoyer->clear();
    return;
  }
  ui->lineEditAEnvoyer->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getAEnvoyer()
{
  strcpy(aEnvoyer,ui->lineEditAEnvoyer->text().toStdString().c_str());
  return aEnvoyer;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setPersonneConnectee(int i,const char* Text)
{
  if (strlen(Text) == 0 )
  {
    switch(i)
    {
        case 1 : ui->lineEditConnecte1->clear(); break;
        case 2 : ui->lineEditConnecte2->clear(); break;
        case 3 : ui->lineEditConnecte3->clear(); break;
        case 4 : ui->lineEditConnecte4->clear(); break;
        case 5 : ui->lineEditConnecte5->clear(); break;
    }
    return;
  }
  switch(i)
  {
      case 1 : ui->lineEditConnecte1->setText(Text); break;
      case 2 : ui->lineEditConnecte2->setText(Text); break;
      case 3 : ui->lineEditConnecte3->setText(Text); break;
      case 4 : ui->lineEditConnecte4->setText(Text); break;
      case 5 : ui->lineEditConnecte5->setText(Text); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getPersonneConnectee(int i)
{
  QLineEdit *tmp;
  switch(i)
  {
    case 1 : tmp = ui->lineEditConnecte1; break;
    case 2 : tmp = ui->lineEditConnecte2; break;
    case 3 : tmp = ui->lineEditConnecte3; break;
    case 4 : tmp = ui->lineEditConnecte4; break;
    case 5 : tmp = ui->lineEditConnecte5; break;
    default : return NULL;
  }

  strcpy(connectes[i],tmp->text().toStdString().c_str());
  return connectes[i];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::ajouteMessage(const char* personne,const char* message)
{
  // Choix de la couleur en fonction de la position
  int i=1;
  bool trouve=false;
  while (i<=5 && !trouve)
  {
      if (getPersonneConnectee(i) != NULL && strcmp(getPersonneConnectee(i),personne) == 0) trouve = true;
      else i++;
  }
  char couleur[40];
  if (trouve)
  {
      switch(i)
      {
        case 1 : strcpy(couleur,"<font color=\"red\">"); break;
        case 2 : strcpy(couleur,"<font color=\"blue\">"); break;
        case 3 : strcpy(couleur,"<font color=\"green\">"); break;
        case 4 : strcpy(couleur,"<font color=\"darkcyan\">"); break;
        case 5 : strcpy(couleur,"<font color=\"orange\">"); break;
      }
  }
  else strcpy(couleur,"<font color=\"black\">");
  if (strcmp(getNom(),personne) == 0) strcpy(couleur,"<font color=\"purple\">");

  // ajout du message dans la conversation
  char buffer[300];
  sprintf(buffer,"%s(%s)</font> %s",couleur,personne,message);
  ui->textEditConversations->append(buffer);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setNomRenseignements(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditNomRenseignements->clear();
    return;
  }
  ui->lineEditNomRenseignements->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getNomRenseignements()
{
  strcpy(nomR,ui->lineEditNomRenseignements->text().toStdString().c_str());
  return nomR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setGsm(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditGsm->clear();
    return;
  }
  ui->lineEditGsm->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setEmail(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditEmail->clear();
    return;
  }
  ui->lineEditEmail->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setCheckbox(int i,bool b)
{
  QCheckBox *tmp;
  switch(i)
  {
    case 1 : tmp = ui->checkBox1; break;
    case 2 : tmp = ui->checkBox2; break;
    case 3 : tmp = ui->checkBox3; break;
    case 4 : tmp = ui->checkBox4; break;
    case 5 : tmp = ui->checkBox5; break;
    default : return;
  }
  tmp->setChecked(b);
  if (b) tmp->setText("Accepté");
  else tmp->setText("Refusé");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::loginOK()
{
  ui->pushButtonLogin->setEnabled(false);
  ui->pushButtonLogout->setEnabled(true);
  ui->lineEditNom->setReadOnly(true);
  ui->lineEditMotDePasse->setReadOnly(true);
  ui->checkBoxNouveau->setEnabled(false);
  ui->pushButtonEnvoyer->setEnabled(true);
  ui->pushButtonConsulter->setEnabled(true);
  ui->pushButtonModifier->setEnabled(true);
  ui->checkBox1->setEnabled(true);
  ui->checkBox2->setEnabled(true);
  ui->checkBox3->setEnabled(true);
  ui->checkBox4->setEnabled(true);
  ui->checkBox5->setEnabled(true);
  ui->lineEditAEnvoyer->setEnabled(true);
  ui->lineEditNomRenseignements->setEnabled(true);
  setTimeOut(TIME_OUT);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::logoutOK()
{
  ui->pushButtonLogin->setEnabled(true);
  ui->pushButtonLogout->setEnabled(false);
  ui->lineEditNom->setReadOnly(false);
  ui->lineEditNom->setText("");
  ui->lineEditMotDePasse->setReadOnly(false);
  ui->lineEditMotDePasse->setText("");
  ui->checkBoxNouveau->setEnabled(true);
  ui->pushButtonEnvoyer->setEnabled(false);
  ui->pushButtonConsulter->setEnabled(false);
  ui->pushButtonModifier->setEnabled(false);
  for (int i=1 ; i<=5 ; i++)
  {
      setCheckbox(i,false);
      setPersonneConnectee(i,"");
  }
  ui->checkBox1->setEnabled(false);
  ui->checkBox2->setEnabled(false);
  ui->checkBox3->setEnabled(false);
  ui->checkBox4->setEnabled(false);
  ui->checkBox5->setEnabled(false);
  setNomRenseignements("");
  setGsm("");
  setEmail("");
  ui->textEditConversations->clear();
  setAEnvoyer("");
  ui->lineEditAEnvoyer->setEnabled(false);
  ui->lineEditNomRenseignements->setEnabled(false);
  setTimeOut(TIME_OUT);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions permettant d'afficher des boites de dialogue /////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::dialogueMessage(const char* titre,const char* message)
{
   QMessageBox::information(this,titre,message);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::dialogueErreur(const char* titre,const char* message)
{
   QMessageBox::critical(this,titre,message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Clic sur la croix de la fenêtre ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::closeEvent(QCloseEvent *event)
{
    // TO DO

    //envoie d'une requete DECONNECT au serveur
    fprintf(stderr,"(CLIENT %d) Envoi requete DECONNECT au serveur\n",getpid());
    MESSAGE disconnectCLIENT;
    disconnectCLIENT.type = 1;
    disconnectCLIENT.expediteur = getpid();
    disconnectCLIENT.requete = DECONNECT;
    if (msgsnd(idQ,&disconnectCLIENT,sizeof(MESSAGE)-sizeof(long),0) == -1)
    {
        perror("(CLIENT) Erreur envoi requete DECONNECT");
        exit(1);
    }
    fprintf(stderr,"Processus %d a envoye un message de DECONNECT au serveur avec succes\n",getpid());



    QApplication::exit();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les boutons ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonLogin_clicked()
{
  MESSAGE loginCLIENT;
  loginCLIENT.type = 1;
  loginCLIENT.expediteur = getpid();
  loginCLIENT.requete = LOGIN;
  loginCLIENT.data1= isNouveauChecked();
  strcpy(loginCLIENT.data2,getNom());
  strcpy(loginCLIENT.texte,getMotDePasse());

  if( msgsnd(idQ,&loginCLIENT,sizeof(MESSAGE)-sizeof(long),0) == -1)
  {
      perror("(CLIENT) Erreur envoi requete LOGIN");
      exit(1);
  }
  fprintf(stderr,"Processus %d a envoye un message d'identification de type LOGIN au serveur avec succes\n",getpid());

}

void WindowClient::on_pushButtonLogout_clicked()
{
  MESSAGE logoutCLIENT;
  logoutCLIENT.type = 1;
  logoutCLIENT.expediteur = getpid();
  logoutCLIENT.requete = LOGOUT;
  if( msgsnd(idQ,&logoutCLIENT,sizeof(MESSAGE)-sizeof(long),0) == -1)
  {
      perror("(CLIENT) Erreur envoi requete LOGOUT");
      exit(1);
  }
  fprintf(stderr,"Processus %d a envoye un message de type LOGOUT au serveur avec succes\n",getpid());
  logoutOK();
  logged = false;
}

void WindowClient::on_pushButtonEnvoyer_clicked()
{
  const char* message = getAEnvoyer();
  if (strlen(message) == 0)
  {
      QMessageBox::critical(this,"Problème...","Message vide...");
      return;
  }
  MESSAGE m;
  m.type = 1;
  m.expediteur = getpid();
  m.requete = SEND;
  strcpy(m.data1,"");
  strncpy(m.texte,texte,sizeof(m.texte)-1);
  m.texte[sizeof(m.texte)-1] = '\0';

  envoieMessage(&m);
  setAEnvoyer("");
}

void WindowClient::on_pushButtonConsulter_clicked()
{
    // TO DO

}

void WindowClient::on_pushButtonModifier_clicked()
{
  // TO DO
  // Envoi d'une requete MODIF1 au serveur
  MESSAGE m;
  // ...

  // Attente d'une reponse en provenance de Modification
  fprintf(stderr,"(CLIENT %d) Attente reponse MODIF1\n",getpid());
  // ...

  // Verification si la modification est possible
  if (strcmp(m.data1,"KO") == 0 && strcmp(m.data2,"KO") == 0 && strcmp(m.texte,"KO") == 0)
  {
    QMessageBox::critical(w,"Problème...","Modification déjà en cours...");
    return;
  }

  // Modification des données par utilisateur
  DialogModification dialogue(this,getNom(),"",m.data2,m.texte);
  dialogue.exec();
  char motDePasse[40];
  char gsm[40];
  char email[40];
  strcpy(motDePasse,dialogue.getMotDePasse());
  strcpy(gsm,dialogue.getGsm());
  strcpy(email,dialogue.getEmail());

  // Envoi des données modifiées au serveur
  // ...
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les checkbox ///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_checkBox1_clicked(bool checked)
{
    const char* nomUtilisateur = getNomRenseignements();
    //ou 
    const char* nomUtilisateur = getPersonneConnectee(1);
    if (checked)
    {
        ui->checkBox1->setText("Accepté");
        envoyerAcceptRefuse(nomUtilisateur,ACCEPT_USER);
    }
    else
    {
        ui->checkBox1->setText("Refusé");
        envoyerAcceptRefuse(nomUtilisateur,REFUSE_USER);
    }
}

void WindowClient::on_checkBox2_clicked(bool checked)
{
    const char* nomUtilisateur = getNomRenseignements();
    //ou 
    const char* nomUtilisateur = getPersonneConnectee(2);
    if (checked)
    {
        ui->checkBox2->setText("Accepté");
        envoyerAcceptRefuse(nomUtilisateur,ACCEPT_USER);
    }
    else
    {
        ui->checkBox2->setText("Refusé");
        envoyerAcceptRefuse(nomUtilisateur,REFUSE_USER);
    }
}

void WindowClient::on_checkBox3_clicked(bool checked)
{
    const char* nomUtilisateur = getNomRenseignements();
    //ou 
    const char* nomUtilisateur = getPersonneConnectee(3);
    if (checked)
    {
        ui->checkBox3->setText("Accepté");
        envoyerAcceptRefuse(nomUtilisateur,ACCEPT_USER);
    }
    else
    {
        ui->checkBox3->setText("Refusé");
        envoyerAcceptRefuse(nomUtilisateur,REFUSE_USER);
    }
}

void WindowClient::on_checkBox4_clicked(bool checked)
{
    const char* nomUtilisateur = getNomRenseignements();
    //ou 
    const char* nomUtilisateur = getPersonneConnectee(4);
    if (checked)
    {
        ui->checkBox4->setText("Accepté");
        envoyerAcceptRefuse(nomUtilisateur,ACCEPT_USER);
    }
    else
    {
        ui->checkBox4->setText("Refusé");
        envoyerAcceptRefuse(nomUtilisateur,REFUSE_USER);
    }
}

void WindowClient::on_checkBox5_clicked(bool checked)
{
    const char* nomUtilisateur = getNomRenseignements();
    //ou 
    const char* nomUtilisateur = getPersonneConnectee(5);
    if (checked)
    {
        ui->checkBox5->setText("Accepté");
        envoyerAcceptRefuse(nomUtilisateur,ACCEPT_USER);
    }
    else
    {
        ui->checkBox5->setText("Refusé");
        envoyerAcceptRefuse(nomUtilisateur,REFUSE_USER);
    }
}
void WindowClient::envoyerAcceptRefuse(const char* nomUtilisateur, int typeRequete)
{
  if(!nomUtilisateur || strlen(nomUtilisateur) == 0) return;

  MESSAGE m;
  m.type = 1;
  m.expediteur = getpid();
  m.requete = typeRequete;
  strcpy(m.data1, nomUtilisateur);

  if( msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1)
  {
      perror("(CLIENT) Erreur envoi requete ACCEPT_USER/REFUSE_USER");
      exit(1);
  }
  fprintf(stderr, "(CLIENT %d) Envoi %s pour %s\n", getpid(),
            (typeRequete == ACCEPT_USER) ? "ACCEPT_USER" : "REFUSE_USER",
            nomUtilisateur);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Handlers de signaux ////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handlerSIGUSR1(int sig)
{
    MESSAGE m,envoye;
    if(msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1)
    {
        perror("(CLIENT) Erreur de msgrcv");
        exit(1);
    }
      switch(m.requete)
      {
        case LOGIN :
                    w->dialogueMessage("Resultat login",m.texte);
                    if(m.data1 == 1){
                        w->loginOK();
                        logged = true;
                        envoye.type = 1;
                        envoye.expediteur = getpid();
                        envoye.requete = CONSULT;
                        envoye.data1 = 1;

                        if( msgsnd(idQ,&envoye,sizeof(MESSAGE)-sizeof(long),0) == -1)
                        {
                            perror("(CLIENT) Erreur envoi requete CONSULT");
                            exit(1);
                        }else{
                            fprintf(stderr,"Processus %d a envoye un message de type CONSULT au serveur avec succes\n",getpid());
                            logged = false;
                            break;
                        } 
                    }

        case ADD_USER :
                    fprintf(stderr,"(CLIENT %d) Reception requete ADD_USER\n",getpid());
                    for(int i =1; i<=5; i++){
                        if(strcmp(w->getPersonneConnectee(i),"") == 0){
                            w->setPersonneConnectee(i,m.data1);
                            break;
                        }
                    }
                    break;

        case REMOVE_USER :
                    fprintf(stderr,"(CLIENT %d) Reception requete REMOVE_USER\n",getpid());
                    for(int i =1; i<=5; i++){
                        if(strcmp(w->getPersonneConnectee(i),m.data1) == 0){
                            w->setPersonneConnectee(i,"");
                            break;
                        }
                    }
                    break;

        case SEND :
                    fprintf(stderr,"(CLIENT %d) Reception requete SEND\n",getpid());
                    w->ajouteMessage(m.data1,m.texte);
                    break;

        case CONSULT :
                  // TO DO
                  break;
        case 
      }
}
