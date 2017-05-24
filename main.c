/*
 * Diagnostic du système
 * - statut Redis
 * - BDD
 * - GPS
 * - SenseHat
 *       gcc -Wall led_matrix.c -o led_matrix
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <time.h>

#include "hiredis/hiredis.h"

// Paramétres de la matrice LED
#define FILEPATH "/dev/fb1"
#define NUM_WORDS 64
#define FILESIZE (NUM_WORDS * sizeof(uint16_t))

// Définition des couleurs
#define RED 0xF800
#define GREEN 0xFF00

// Définition de la LED de base
#define LED_BASE 56

int main(void)
{

	// Variables utilisées pour la matrice LED
    int FrameBufferMatrix;
    uint16_t *map;
    uint16_t *p;
    struct fb_fix_screeninfo fix_info;
    int flag = 1;
    int i = 0;
    long longNbrSecondeRedis = 0;
    long longNbrSecondeCurrent = 0;
    char *errorLong;
    
	// Variables utilisées pour Redis
    redisContext *c;
    redisReply *reply;     

    // Ouverture du buffer
    FrameBufferMatrix = open(FILEPATH, O_RDWR);
    if (FrameBufferMatrix == -1) {
		perror("Erreur : Impossible d ouvrir FILEPATH");
		exit(EXIT_FAILURE);
    }

    // Information de l'ecran
    if (ioctl(FrameBufferMatrix, FBIOGET_FSCREENINFO, &fix_info) == -1) {
		perror("Erreur : Informations Ecran");
		close(FrameBufferMatrix);
		exit(EXIT_FAILURE);
    }
    
    // Nous sommes bien sur l'ecran du SenseHat
    if (strcmp(fix_info.id, "RPi-Sense FB") != 0) {
		printf("%s\n", "RPi-Sense FB non trouve");
		close(FrameBufferMatrix);
		exit(EXIT_FAILURE);
    }

    /* map the led frame buffer device into memory */
    map = mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, FrameBufferMatrix, 0);
    if (map == MAP_FAILED) {
		close(FrameBufferMatrix);
		perror("Error mmapping the file");
		exit(EXIT_FAILURE);
    }
    p = map;

    // On éteint la matrice le LED
    memset(map, 0, FILESIZE);

	// 0x0 : Vert, le système est lancé
	*(p + LED_BASE) = GREEN;
	
	// Connexion à Redis
	struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout("127.0.0.1", 6379, timeout);
    if (c == NULL || c->err) {
        if (c) {
            redisFree(c);
        }
        
        // 0x1 : Rouge, pas de connexion Redis
        *(p + LED_BASE +1) = RED;
        exit(EXIT_FAILURE);
	}
    // 0x1 : Vert, on est bon pour Redis
	*(p + LED_BASE +1) = GREEN;
	

	while (flag) {
		
	// Récupération du statut du GPS
		reply = redisCommand(c,"GET current_GPS_Status");
		
		if (reply->type == REDIS_REPLY_STRING && strcmp(reply->str,"1") == 0){
			// 0x3 : Vert, GPS actif
			*(p + LED_BASE +3) = GREEN;	
		} else {
			// 0x3 : Rouge, GPS inactif
			*(p + LED_BASE +3) = RED;	
		}
		freeReplyObject(reply);
		
	// Récupération du time GPS
		reply = redisCommand(c,"GET current_time_gps");
		if (reply->type == REDIS_REPLY_STRING && strcmp(reply->str,"(null)") != 0){
			longNbrSecondeRedis = strtol(reply->str, &errorLong, 0);
			longNbrSecondeCurrent = time(NULL);
			if (longNbrSecondeCurrent - longNbrSecondeRedis > 1) {
				*(p + LED_BASE +4) = RED;	
			} else {
				*(p + LED_BASE +4) = GREEN;	
			}
		} else {
			*(p + LED_BASE +4) = RED;	
		}
		freeReplyObject(reply);
		
	// Récupération du time IMU
		reply = redisCommand(c,"GET current_time_imu");
		if (reply->type == REDIS_REPLY_STRING && strcmp(reply->str,"(null)") != 0){
			longNbrSecondeRedis = strtol(reply->str, &errorLong, 0);
			longNbrSecondeCurrent = time(NULL);
			if (longNbrSecondeCurrent - longNbrSecondeRedis > 1) {
				*(p + LED_BASE +5) = RED;	
			} else {
				*(p + LED_BASE +5) = GREEN;	
			}
		} else {
			*(p + LED_BASE +5) = RED;
		}
		freeReplyObject(reply);
		
	// Un vol est-il en cours ?
		reply = redisCommand(c,"GET current_flight_inprogress");
		if (reply->type == REDIS_REPLY_STRING && strcmp(reply->str,"1") == 0){
			*(p + LED_BASE +2) = GREEN;	
		} else {
			*(p + LED_BASE +2) = RED;
		}
		freeReplyObject(reply);
		
	// Pour l'instant, on quitte la boucle au bout d'un lapse de temps
		usleep(250000);
		if (i > 80) {
			flag = 0;
		}
		i++;
	/////
	}

    // On éteint la matrice le LED
    memset(map, 0, FILESIZE);

    // On ferme le map de la matrice
    if (munmap(map, FILESIZE) == -1) {
		perror("Error un-mmapping the file");
    }
    close(FrameBufferMatrix);

    return 0;
}
