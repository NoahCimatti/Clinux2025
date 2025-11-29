#include "FichierUtilisateur.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int estPresent(const char* nom)
{

  int fd, position = 0, nbUtilisateur = 0;
  UTILISATEUR copie;
  
  if((fd = open(FICHIER_UTILISATEURS,O_RDONLY, 0644)) == -1) 
  {
    perror("erreur d'ouverture en lecture");
    return -1;
  }

  nbUtilisateur = (lseek(fd,0,SEEK_END)/sizeof(UTILISATEUR));
  lseek(fd,0,SEEK_SET);

  for(int i = 0;i < nbUtilisateur; i++)
  {
    read(fd,&copie,sizeof(UTILISATEUR));
    if(strcmp(copie.nom,nom) == 0)
    {
      position = i + 1;
      break;
    }
  }

  close(fd);

  return position;
}


////////////////////////////////////////////////////////////////////////////////////
int hash(const char* motDePasse)
{
  int nbCaractere = strlen(motDePasse), valeurHash = 0;

  for (int i = 0; i < nbCaractere; i++)
  {
    valeurHash += (i + 1) * motDePasse[i]; 
  }

  return valeurHash % 97;
}

////////////////////////////////////////////////////////////////////////////////////
int ajouteUtilisateur(const char* nom, const char* motDePasse)
{
  if(((strlen(nom) == 0 || strlen(nom) > 20)) || ((strlen(motDePasse) == 0 || strlen(motDePasse) > 20))) return -1;

  int fd;

  if ((fd = open(FICHIER_UTILISATEURS, O_WRONLY | O_APPEND | O_CREAT, 0644)) == -1)
  {
    perror("erreur d'ouverture");
    return -1;
  }

  UTILISATEUR enregistrement;

  strcpy(enregistrement.nom, nom);
  enregistrement.hash = hash(motDePasse);

  write(fd,&enregistrement,sizeof(UTILISATEUR));
  close(fd);
  return 1;
}

////////////////////////////////////////////////////////////////////////////////////
int verifieMotDePasse(int pos, const char* motDePasse)
{
  int fd, valeurHash = 0;
  UTILISATEUR enregistrement;

  if((fd = open(FICHIER_UTILISATEURS,O_RDONLY)) == -1)
  {
    perror("erreur d'ouverture verifieMotDePasse");
    return -1;
  }

  if (pos <= 0)
  {
    printf("Position invalide dans verifieMotDePasse\n");
    close(fd);
    return -1;
  }

  lseek(fd,(pos - 1) * sizeof(UTILISATEUR), SEEK_SET);
  read(fd,&enregistrement,sizeof(UTILISATEUR));
  close(fd);

  valeurHash = hash(motDePasse);

  if (enregistrement.hash == valeurHash) return 1;
  
  return 0;
}

////////////////////////////////////////////////////////////////////////////////////
int listeUtilisateurs(UTILISATEUR *vecteur) // le vecteur doit etre suffisamment grand
{
  int fd, nbUtilisateur = 0;
  UTILISATEUR copie;

  if((fd = open(FICHIER_UTILISATEURS,O_RDONLY)) == -1)
  {
    perror("erreur d'ouverture listeUtilisateurs\n");
    return -1;
  }

  
  nbUtilisateur = (lseek(fd,0,SEEK_END) / sizeof(UTILISATEUR));
  lseek(fd,0,SEEK_SET);

  for (int i = 0; i < nbUtilisateur; i++)
  {
    read(fd,&copie,sizeof(UTILISATEUR));
    strcpy(vecteur[i].nom,copie.nom);
    vecteur[i].hash = copie.hash;
  }

  close(fd);

  return nbUtilisateur;
}