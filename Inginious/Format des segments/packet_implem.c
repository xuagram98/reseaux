#include "packet_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <arpa/inet.h>

struct __attribute__((__packed__)) pkt {
	char * payload;
  // Ne pas oublier d'inverser le sens des bits
  uint8_t window:5; // Encode sur 5 bits
  uint8_t tr:1; // Encode sur 1 bit
  uint8_t type:2; // Encode sur 2 bits
  uint8_t seqnum; // Encode sur 8 bits
  uint16_t length; // Encode sur 16 bits
  uint32_t timestamp; // Encode sur 32 bits (4 octets)
  uint32_t crc1; // Encode sur 32 bits (4 octets)
  uint32_t crc2; // Encode sur 32 bits (4 octets)
};

/* Alloue et initialise une struct pkt
 * @return: NULL en cas d'erreur */
pkt_t* pkt_new()
{
	pkt_t * new = (pkt_t *) malloc(sizeof(pkt_t));
  if (new == NULL){
    fprintf(stderr, "Erreur du malloc");
    return NULL;
  }
  new->window = 1; // Par definition, on fait commencer la fenetre à 1
  new->tr = 0;
  new->type = 1;
  new->seqnum = 0;
  new->length = 0;
  new->timestamp = 0;
  new->crc1 = 0;
  new->crc2 = 0;
  new->payload = (char *)malloc(512 * sizeof(char));
  if (new->payload == NULL){
    fprintf(stderr, "Erreur du malloc");
    return NULL;
  }
  return new;
}

/* Libere le pointeur vers la struct pkt, ainsi que toutes les
 * ressources associees*/
void pkt_del(pkt_t *pkt)
{
    free (pkt->payload);
    free (pkt);
}

/*
 * Decode des donnees recues et cree une nouvelle structure pkt.
 * Le paquet recu est en network byte-order.
 * La fonction verifie que:
 * - Le CRC32 du header recu est le mÃªme que celui decode a la fin
 *   du header (en considerant le champ TR a 0)
 * - S'il est present, le CRC32 du payload recu est le meme que celui
 *   decode a la fin du payload
 * - Le type du paquet est valide
 * - La longueur du paquet et le champ TR sont valides et coherents
 *   avec le nombre d'octets recus.
 *
 * @data: L'ensemble d'octets constituant le paquet recu
 * @len: Le nombre de bytes recus
 * @pkt: Une struct pkt valide
 * @post: pkt est la representation du paquet recu
 *
 * @return: Un code indiquant si l'operation a reussi ou representant
 *         l'erreur rencontree.
 */
pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
  if (len == 0){ // Le paquet est incoherent
    return E_UNCONSISTENT;
  }
  else if (len < 4){ // Il n'y a pas de header car il est encode sur 4 bytes
    return E_NOHEADER;
  }
  else {
	uint16_t length = pkt_get_length(pkt);
  uint32_t crc1 = pkt_get_crc1(pkt);
  uint32_t crc2 = pkt_get_crc2(pkt);
  size_t indice_header = 4; // Indice ou on doit terminer de copier le header
  size_t indice_crc1 = *(data + 8);// Indice du data ou on doit commencer à copier le crc1
  size_t indice_timestamp = *(data + 4); // Indice du data ou on doit commencer à copier le timestamp
  size_t indice_payload = *(data + 12);
  size_t i;

  // On copie le header
  for (i = 0; i < 4; i++){
    memcpy((void *)data, pkt, len);
  }

  // On copie le timestamp
  for (i = 4; i < 6; i++){
    memcpy((void *)indice_timestamp, pkt, len);
  }

  // On decode le crc1
  uLong new_crc1 = crc32(0L, Z_NULL, 0);
  new_crc1 = crc32(new_crc1,(Bytef*) data, 4);
  if (crc1 != new_crc1){ // Si le crc1 n'est pas verifie
    return E_CRC;
  }

  for (i = 6; i < 8; i++){
    memcpy((void *) indice_crc1, &crc1, sizeof(crc1));
  }
  pkt_set_crc1(pkt, crc1);

  // On decode le payload
  for (i = 12; i < 21; i++){
    memcpy((void *)indice_payload, pkt->payload, length);
  }

  // On decode le crc2
  if (pkt_get_length(pkt) <= 0){ // Si le paquet ne contient pas de payload
    if(pkt_get_tr(pkt) != 0){ // Si le paquet est tronque
      return E_TR;
    }
    return E_LENGTH;
  }
  const char * buffer = pkt_get_payload(pkt);
  uLong new_crc2 = crc32(0L, Z_NULL, 0);
  new_crc2 = crc32(new_crc2,(const Bytef *) buffer, pkt_get_length(pkt));
  if (crc2 != new_crc2){ // Si le crc2 n'est pas verifie
    return E_CRC;
  }
  for (i = len-4; i < len; i++){
    memcpy((*data + 12 + pkt_get_length), &crc2, sizeof(crc2));
  }
  pkt_set_crc2(pkt, crc2);
}
return PKT_OK;
}

/*
 * Encode une struct pkt dans un buffer, prÃªt a Ãªtre envoye sur le reseau
 * (c-a-d en network byte-order), incluant le CRC32 du header et
 * eventuellement le CRC32 du payload si celui-ci est non nul.
 *
 * @pkt: La structure a encoder
 * @buf: Le buffer dans lequel la structure sera encodee
 * @len: La taille disponible dans le buffer
 * @len-POST: Le nombre de d'octets ecrit dans le buffer
 * @return: Un code indiquant si l'operation a reussi ou E_NOMEM si
 *         le buffer est trop petit.
 */
pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
  // Gerer le header
  const char * payload = pkt_get_payload(pkt);
  uint8_t window = pkt_get_window(pkt);
  uint8_t type = pkt_get_type(pkt);
  uint8_t tr = pkt_get_tr(pkt);
  // window + type + tr = 1 byte
  //uint8_t seqnum = pkt_get_seqnum(pkt);  1 byte
  uint16_t length = pkt_get_length(pkt); // 2 bytes
  // + 4 bytes timestamp + 4 bytes crc1 = 12 bytes
  size_t longueur = pkt_get_length(pkt);
  if (*len < longueur + 12){
    return E_NOMEM;
  }
  // On encode le header
  uint8_t premier_byte = type<<6 | tr<<5; // premier byte = 0110 0000
  buf[0] = premier_byte | window; // Pour completer le byte
  char * buffer = (char *) pkt;
  size_t i;

  for(i = 1; i < 8 ; i++){
    buf[i] = buffer[i];
  }
  // Gerer les CRC
  uLong crc1 = crc32(0L, Z_NULL, 0);
	crc1 = crc32(crc1,(const Bytef *) buf, 8);

	// On encode le crc1
	for(i = 0 ; i<length; i++){
		buf[8+i] = payload[i]; // Le crc1 commence apres 8 bytes
	}
	if(pkt_get_tr(pkt) == 0){ // Si le paquet n'est pas tronque --> crc2
		uLong crc2 = crc32(0L, Z_NULL, 0);
		crc2 = crc32(crc2,((const Bytef *)buf), length);
		// On encode le crc2
		(*(buf+length+12)) = crc2; // Le crc2 apres 12 bytes + la longueur du payload
	}

  // On encode le payload
	for(i = 0; i < 9; i++){
		buf[12+i] = payload[i]; // Le payload commence apres 12 bytes
	}
  return PKT_OK;
}

ptypes_t pkt_get_type  (const pkt_t * pkt)
{
	return pkt->type;
}

uint8_t  pkt_get_tr(const pkt_t * pkt)
{
	return pkt->tr;
}

uint8_t  pkt_get_window(const pkt_t * pkt)
{
	return pkt->window;
}

uint8_t  pkt_get_seqnum(const pkt_t * pkt)
{
	return pkt->seqnum;
}

uint16_t pkt_get_length(const pkt_t * pkt)
{
	return pkt->length;
}

uint32_t pkt_get_timestamp   (const pkt_t * pkt)
{
	return pkt->timestamp;
}

uint32_t pkt_get_crc1   (const pkt_t * pkt)
{
	return pkt->crc1;
}

uint32_t pkt_get_crc2   (const pkt_t * pkt)
{
	return pkt->crc2;
}

const char* pkt_get_payload(const pkt_t * pkt)
{
	if ((pkt->length) == 0){
    return NULL;
  }
  return pkt->payload;
}


pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
	if(type == 1 || type == 2 || type == 3){
    pkt->type = type;
    return PKT_OK;
  }
  return E_TYPE;
}

pkt_status_code pkt_set_tr(pkt_t *pkt, const uint8_t tr)
{
	if (tr != 0 && tr != 1){
    return E_TR;
  }
  pkt->tr = tr;
  return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
	if (window > MAX_WINDOW_SIZE){
    return E_WINDOW;
  }
  pkt->window = window;
  return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
	pkt->seqnum = seqnum;
  return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
	if (length > MAX_PAYLOAD_SIZE){
    return E_LENGTH;
  }
  pkt->length = length;
  return PKT_OK;
}

pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp)
{
	pkt->timestamp = timestamp;
  return PKT_OK;
}

pkt_status_code pkt_set_crc1(pkt_t *pkt, const uint32_t crc1)
{
	pkt->crc1 = crc1;
  return PKT_OK;
}

pkt_status_code pkt_set_crc2(pkt_t *pkt, const uint32_t crc2)
{
	pkt->crc2 = crc2;
  return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt, const char *data, const uint16_t length)
{
	if (length > MAX_PAYLOAD_SIZE){
    return E_LENGTH;
  }
  pkt->payload = realloc(pkt->payload, length);
  memcpy(pkt->payload, data, length);
  return PKT_OK;
}