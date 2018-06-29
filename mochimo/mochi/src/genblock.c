/* genblock.c  Genesis Block Creator
 *
 * Copyright (c) 2018 by Adequate Systems, LLC.  All Rights Reserved.
 * See LICENSE.TXT   **** NO WARRANTY ****
 *
 * The Mochimo Project System Software
 *
 * Date: 1 March 2018
 *
*/

#include "config.h"
#include "mochimo.h"

#define EXCLUDE_NODES   /* exclude Nodes[], ip, and socket data */
#include "data.c"

#include "error.c"
#include "crc16.c"
#include "rand.c"
#include "add64.c"
#include "util.c"
#include "daemon.c"

void bail(char *message)
{
   printf("genblock: bailing out: %s", message);
   exit(1);
}


int main(int argc, char **argv)
{
   static BTRAILER bt;
   static LENTRY le;
   word32 hdrlen;           /* header length for Genesis Block */
   SHA256_CTX bctx;
   FILE *gfp, *lfp;

   fix_signals();

   if(argc != 3) {
      printf("\nusage: genblock bc/b000...bc ledger.entry\n\n");
      exit(1);
   }

   /* open ledger entry read-only */
   if((lfp = fopen(argv[2], "rb")) == NULL)
      bail("Cannot open ledger entry");

   hdrlen = sizeof(LENTRY);
   if(fread(&le, 1, hdrlen, lfp) != hdrlen)
      bail("ledger entry I/O error");
   fclose(lfp);

   gfp = fopen(argv[1], "wb");

   /* Add length of LENTRY to length of header length field. */
   hdrlen += 4;

   /* Begin the Neo-Genesis block by writing the header length to it. */
   if(fwrite(&hdrlen, 1, 4, gfp) != 4) {
badneo:
      bail("Genesis Block I/O error");
   }
   if(fwrite(&le, 1, sizeof(LENTRY), gfp) != sizeof(LENTRY)) goto badneo;

   sha256_init(&bctx);   /* begin entire block hash */
   /* ... with the header length field. */
   sha256_update(&bctx, (byte *) &hdrlen, 4);
   sha256_update(&bctx, (byte *) &le, sizeof(LENTRY));

   /* Fix-up block trailer and write to neo-block */
   sha256_update(&bctx, (byte *) &bt, sizeof(BTRAILER) - HASHLEN);
   sha256_final(&bctx, bt.bhash);
   if(fwrite(&bt, 1, sizeof(BTRAILER), gfp) != sizeof(BTRAILER))
      goto badneo;
   if(ferror(gfp)) goto badneo;
   fclose(gfp);
   return 0;  /* success */
}
