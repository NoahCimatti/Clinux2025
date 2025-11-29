#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <mysql.h>
#include <setjmp.h>
#include "protocole.h" // contient la cle et la structure d'un message
#include "Utilisateur/FichierUtilisateur.h"
#define MAX_CONNEXIONS 6


int idQ,idShm,idSem;
TAB_CONNEXIONS *tab;

void afficheTab();

MYSQL* connexion;


int main()
{

  creationRessource();

  // Creation du processus Publicite

  int i,k,j;
  MESSAGE m;
  MESSAGE reponse;
  char requete[200];
  MYSQL_RES  *resultat;
  MYSQL_ROW  tuple;
  PUBLICITE pub;

  while(1)
  {
  	fprintf(stderr,"(SERVEUR %d) Attente d'une requete...\n",getpid());
    if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),1,0) == -1)
    {
      perror("(SERVEUR) Erreur de msgrcv");
      msgctl(idQ,IPC_RMID,NULL);
      exit(1);
    }

    switch(m.requete)
    {
      case CONNECT :  
                      fprintf(stderr,"(SERVEUR %d) Requete CONNECT reçue de %d\n",getpid(),m.expediteur);
                      gestionRequeteConnect(&m);
                      break; 

      case DECONNECT :  
                      fprintf(stderr,"(SERVEUR %d) Requete DECONNECT reçue de %d\n",getpid(),m.expediteur);
                      gestionRequeteDeconnect(&m);
                      break; 

      case LOGIN :  
                      fprintf(stderr,"(SERVEUR %d) Requete LOGIN reçue de %d : --%s--%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2,m.texte);
                      if(sem_wait(0,NON_BOQUANT)==0){
                        gestionRequeteLogin(&m,fdPipe[1]);
                        sem_signal(0);
                      }else{
                        envoyerReponseBusy(m.expediteur);
                      }
                      break; 

      case LOGOUT :  
                      fprintf(stderr,"(SERVEUR %d) Requete LOGOUT reçue de %d\n",getpid(),m.expediteur);
                      gestionRequeteLogout(&m);
                      break;

      case ACCEPT_USER :
                      fprintf(stderr,"(SERVEUR %d) Requete ACCEPT_USER reçue de %d\n",getpid(),m.expediteur);
                      int pidCible = recherchePidParNom(m.data1);
                      if(pidCible !=0){
                        ajouteAutres(m.expediteur, pidCible);
                      }
                      break;

      case REFUSE_USER :
                      fprintf(stderr,"(SERVEUR %d) Requete REFUSE_USER reçue de %d\n",getpid(),m.expediteur);
                      int pidCible = recherchePidParNom(m.data1);
                      if(pidCible !=0){
                        supprimeAutres(m.expediteur, pidCible);
                      }
                      break;

      case SEND :  
                      fprintf(stderr,"(SERVEUR %d) Requete SEND reçue de %d\n",getpid(),m.expediteur);
                      gestionRequeteSend(&m);
                      break; 

      case UPDATE_PUB :
                      fprintf(stderr,"(SERVEUR %d) Requete UPDATE_PUB reçue de %d\n",getpid(),m.expediteur);
                      break;

      case CONSULT :
                      fprintf(stderr,"(SERVEUR %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);
                      break;

      case MODIF1 :
                      fprintf(stderr,"(SERVEUR %d) Requete MODIF1 reçue de %d\n",getpid(),m.expediteur);
                      break;

      case MODIF2 :
                      fprintf(stderr,"(SERVEUR %d) Requete MODIF2 reçue de %d\n",getpid(),m.expediteur);
                      break;

      case LOGIN_ADMIN :
                      fprintf(stderr,"(SERVEUR %d) Requete LOGIN_ADMIN reçue de %d\n",getpid(),m.expediteur);
                      break;

      case LOGOUT_ADMIN :
                      fprintf(stderr,"(SERVEUR %d) Requete LOGOUT_ADMIN reçue de %d\n",getpid(),m.expediteur);
                      break;

      case NEW_USER :
                      fprintf(stderr,"(SERVEUR %d) Requete NEW_USER reçue de %d : --%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2);
                      break;

      case DELETE_USER :
                      fprintf(stderr,"(SERVEUR %d) Requete DELETE_USER reçue de %d : --%s--\n",getpid(),m.expediteur,m.data1);
                      break;

      case NEW_PUB :
                      fprintf(stderr,"(SERVEUR %d) Requete NEW_PUB reçue de %d\n",getpid(),m.expediteur);
                      break;
    }
    afficheTab();
  }
}

void afficheTab()
{
  fprintf(stderr,"Pid Serveur 1 : %d\n",tab->pidServeur1);
  fprintf(stderr,"Pid Serveur 2 : %d\n",tab->pidServeur2);
  fprintf(stderr,"Pid Publicite : %d\n",tab->pidPublicite);
  fprintf(stderr,"Pid Admin     : %d\n",tab->pidAdmin);
  for (int i=0 ; i<6 ; i++)
    fprintf(stderr,"%6d -%20s- %6d %6d %6d %6d %6d - %6d\n",tab->connexions[i].pidFenetre,
                                                      tab->connexions[i].nom,
                                                      tab->connexions[i].autres[0],
                                                      tab->connexions[i].autres[1],
                                                      tab->connexions[i].autres[2],
                                                      tab->connexions[i].autres[3],
                                                      tab->connexions[i].autres[4],
                                                      tab->connexions[i].pidModification);
  fprintf(stderr,"\n");
}
void creationRessource()
{
    // Connection à la BD
  connexion = mysql_init(NULL);
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
  {
    fprintf(stderr,"(SERVEUR) Erreur de connexion à la base de données...\n");
    exit(1);  
  }

  // Armement des signaux

  // Creation des ressources
  fprintf(stderr,"(SERVEUR %d) Creation de la file de messages\n",getpid());
  if ((idQ = msgget(CLE,IPC_CREAT | IPC_EXCL | 0600)) == -1)  // CLE definie dans protocole.h
  {
    perror("(SERVEUR) Erreur de msgget");
    exit(1);
  }

  // Initialisation du tableau de connexions
  fprintf(stderr,"(SERVEUR %d) Initialisation de la table des connexions\n",getpid());
  tab = (TAB_CONNEXIONS*) malloc(sizeof(TAB_CONNEXIONS)); 

  for (int i=0 ; i<6 ; i++)
  {
    tab->connexions[i].pidFenetre = 0;
    strcpy(tab->connexions[i].nom,"");
    for (int j=0 ; j<5 ; j++) tab->connexions[i].autres[j] = 0;
    tab->connexions[i].pidModification = 0;
  }
  tab->pidServeur1 = getpid();
  tab->pidServeur2 = 0;
  tab->pidAdmin = 0;
  tab->pidPublicite = 0;

  afficheTab();

}

void envoieMessage(MESSAGE* message)
{
  if (msgsnd(idQ,message,sizeof(MESSAGE)-sizeof(long),0) == -1)
  {
    perror("Erreur de msgsnd");
    liberationRessource();
  }
}
void liberationRessource()
{
  fprintf(stderr,"(Serveur) Liberation des ressources\n");
  kill(tab->pidPublicite, SIGINT);
  msgctl(idQ,IPC_RMID,NULL);
  shmctl(idShm,IPC_RMID,NULL);
  semctl(idSem,0,IPC_RMID);
  close(fdPipe[1]);
  exit(1);
}

void gestionRequeteConnect(MESSAGE *requeteCONNECT){
    int i, sortie;

    for(int=0, sortie =0; i<MAX_CONNEXIONS && sortie ==0;i++){
      if(tab->connexions[i].pidFenetre ==0){
        tab->connexions[i].pidFenetre = requeteCONNECT->expediteur;
        sortie =1;
      }
    }

    if(sortie ==0){
      fprintf(stderr,"(SERVEUR %d) Nombre maximum de connexions atteint. Rejet de la connexion de %d\n",getpid(),requeteCONNECT->expediteur);
      kill(requeteCONNECT->expediteur,SIGINT);
    }
}

void gestionRequeteDeconnect(MESSAGE *requeteDECONNECT){
    int i, sortie;

    for(int=0, sortie =0; i<MAX_CONNEXIONS && sortie ==0;i++){
      if(tab->connexions[i].pidFenetre == requeteDECONNECT->expediteur){
        tab->connexions[i].pidFenetre =0;
        strcpy(tab->connexions[i].nom,"");
        for (int j=0 ; j<5 ; j++) tab->connexions[i].autres[j] = 0;
        tab->connexions[i].pidModification = 0;
        sortie =1;
      }
    }
}

void gestionRequeteLogin(MESSAGE *requeteLOGIN, int fdPipe) {
    char resultat[200];
    bool loginOK = false;
    int i, j;
    MESSAGE reponse;

    loginOK = rechercheFichierClient(requeteLOGIN->data1, requeteLOGIN->data2, requeteLOGIN->texte, resultat);

    reponse.type = requeteLOGIN->expediteur;
    reponse.requete = LOGIN;
    reponse.expediteur = getpid(); // serveur comme expéditeur
    strcpy(reponse.texte, resultat);
    reponse.data1[0] = loginOK ? '1' : '0'; // succès ou échec

    if (loginOK) {
        // Stocker le nom dans la table de connexions
        for (i = 0; i < MAX_CONNEXIONS; i++) {
            if (tab->connexions[i].pidFenetre == requeteLOGIN->expediteur) {
                strcpy(tab->connexions[i].nom, requeteLOGIN->data2);
                break;
            }
        }

        // Envoyer ADD_USER aux utilisateurs déjà connectés
        for (i = 0; i < MAX_CONNEXIONS; i++) {
            if (tab->connexions[i].pidFenetre != 0 && tab->connexions[i].pidFenetre != requeteLOGIN->expediteur) {
                MESSAGE m;
                m.type = tab->connexions[i].pidFenetre;
                m.requete = ADD_USER;
                m.expediteur = getpid();
                strcpy(m.data1, requeteLOGIN->data2);
                envoieMessage(&m);
            }
        }

        // Envoyer ADD_USER pour les utilisateurs déjà connectés vers le nouvel utilisateur
        for (i = 0; i < MAX_CONNEXIONS; i++) {
            if (tab->connexions[i].pidFenetre != 0 && tab->connexions[i].pidFenetre != requeteLOGIN->expediteur) {
                MESSAGE m;
                m.type = requeteLOGIN->expediteur;
                m.requete = ADD_USER;
                m.expediteur = getpid();
                strcpy(m.data1, tab->connexions[i].nom);
                envoieMessage(&m);
            }
        }
    }
    envoieMessage(&reponse);
    kill(requeteLOGIN->expediteur, SIGUSR1);
}

void gestionRequeteSend(MESSAGE* requeteSEND) {
  int i, j;
  int pidEmetteur = requeteSEND->expediteur;
  int indexEmetteur = -1;

  // chercher l'index de l'émetteur dans le tableau
  for (i = 0; i < MAX_CONNEXIONS; i++) {
      if (tab->connexions[i].pidFenetre == pidEmetteur) {
          indexEmetteur = i;
          break;
      }
  }

  if (indexEmetteur == -1) {
      fprintf(stderr, "(SERVEUR %d) Emetteur %d non trouvé dans les connexions\n", getpid(), pidEmetteur);
      return;
  }

  //Préparer le message à envoyer aux utilisateurs acceptés
  MESSAGE m;
  m.requete = SEND;
  m.expediteur = getpid();
  strcpy(m.data1, tab->connexions[indexEmetteur].nom);
  strcpy(m.texte, requeteSEND->texte);

  for (int j = 0; j < 5; j++) {
      if (tab->connexions[indexEmetteur].autres[j] != 0) {
          m.type = tab->connexions[indexEmetteur].autres[j];
          envoieMessage(&m);
          kill(tab->connexions[indexEmetteur].autres[j], SIGUSR1);
      }
  }
  fprintf(stderr, "(SERVEUR %d) Message de %s envoyé aux utilisateurs acceptés\n", getpid(), tab->connexions[indexEmetteur].nom);
}

void gestionRequeteLogout(MESSAGE* requeteLOGOUT) {
    int i, j;
    MESSAGE reponse;

    // Trouver la fenêtre qui se déconnecte
    char nomUtilisateur[20] = "";
    int pidLogout = requeteLOGOUT->expediteur;

    for (i = 0; i < MAX_CONNEXIONS; i++) {
        if (tab->connexions[i].pidFenetre == pidLogout) {
            strcpy(nomUtilisateur, tab->connexions[i].nom);
            // Remettre la ligne de l'utilisateur à zéro
            tab->connexions[i].pidFenetre = 0;
            strcpy(tab->connexions[i].nom, "");
            for (j = 0; j < 5; j++) tab->connexions[i].autres[j] = 0;
            tab->connexions[i].pidModification = 0;
            break;
        }
    }

    // Supprimer ce PID de toutes les autres lignes
    for (i = 0; i < MAX_CONNEXIONS; i++) {
        for (j = 0; j < 5; j++) {
            if (tab->connexions[i].autres[j] == pidLogout) {
                tab->connexions[i].autres[j] = 0;
            }
        }
    }

    // Informer les autres utilisateurs de la déconnexion
    for (i = 0; i < MAX_CONNEXIONS; i++) {
        if (tab->connexions[i].pidFenetre != 0) {
            MESSAGE m;
            m.type = tab->connexions[i].pidFenetre;
            m.requete = REMOVE_USER;
            m.expediteur = getpid();
            strcpy(m.data1, nomUtilisateur);
            envoieMessage(&m);
        }
    }
}

bool rechercheFichierClient(int data1, char data2[20], char texte[200],char resultat[200]){
  char nom[20], mdp[20];
  int newUser, position;
  bool loginOK;

  strcpy(nom, data2);
  strcpy(mdp, texte);
  newUser = data1;
  position = estPresent(nom);

  if(newUser){
    if(position > 0){
      sprintf(resultat,"%s","Utilisateur déjà existant");
      loginOK = false;
    }
    else{
      int res = ajouteUtilisateur(nom,mdp);
      if(res == 1){
        sprintf(resultat,"%s","Mot de passe ou utilisateur incorrect");
        loginOK = false;
    }
    else{
      sprintf(resultat,"%s","Utilisateur ajouté avec succès : Bienvenue");
      loginOK = true;
      }
    }
  }
  if(position == 0){
    sprintf(resultat,"%s","Utilisateur inconnu");
    loginOK = false;
  }
  else{
    if(verifieMotDePasse(position,mdp) == 1){
      sprintf(resultat,"%s","Login réussi : Bienvenue");
      loginOK = true;
    }
    else{
      if(verifieMotDePasse(position,mdp) == 0){
        sprintf(resultat,"%s","Mot de passe incorrect");
        loginOK = false;
      }
      else{
        sprintf(resultat,"%s","Erreur lors de la vérification du mot de passe");
        loginOK = false;
      }
    }
  }
  return loginOK;
}
// Ajouter un PID dans le tableau 'autres' d'un utilisateur
void ajouteAutres(int pidExp, int pidCible) {
    for (int i = 0; i < MAX_CONNEXIONS; i++) {
        if (tab->connexions[i].pidFenetre == pidExp) {
            for (int j = 0; j < 5; j++) {
                if (tab->connexions[i].autres[j] == 0) {
                    tab->connexions[i].autres[j] = pidCible;
                    return;
                }
            }
        }
    }
}

// Supprimer un PID dans le tableau 'autres' d'un utilisateur
void supprimeAutres(int pidExp, int pidCible) {
    for (int i = 0; i < MAX_CONNEXIONS; i++) {
        if (tab->connexions[i].pidFenetre == pidExp) {
            for (int j = 0; j < 5; j++) {
                if (tab->connexions[i].autres[j] == pidCible) {
                    tab->connexions[i].autres[j] = 0;
                    return;
                }
            }
        }
    }
}

// Trouver le PID d'un utilisateur par son nom
int recherchePidParNom(char* nom) {
    for (int i = 0; i < MAX_CONNEXIONS; i++) {
        if (strcmp(tab->connexions[i].nom, nom) == 0) {
            return tab->connexions[i].pidFenetre;
        }
    }
    return 0; // pas trouvé
}