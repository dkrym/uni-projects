/*
 * Upravil: David Krym
 */

/******************************************************************************
 * Projekt - Zaklady pocitacove grafiky - IZG
 * spanel@fit.vutbr.cz
 *
 * $Id:$
 *
 * Opravy a modifikace:
 * -
 */

#ifndef Student_H
#define Student_H

/******************************************************************************
 * Includes
 */

#include "render.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Studentsky renderer, do ktereho muzete dopisovat svuj kod
 */

/* Jadro vaseho rendereru */
typedef struct S_StudentRenderer
{
    /* Kopie default rendereru
     * Typicky trik jak implementovat "dedicnost" znamou z C++
     * Ukazatel na strukturu lze totiz pretypovat na ukazatel
     * na prvni prvek struktury a se strukturou S_StudentRenderer
     * tak pracovat jako s S_Renderer... */
    S_Renderer  base;

    /* Zde uz muzete doplnovat svuj kod dle libosti */

    /* ukazatel na vytvareny z-buffer */
    double *z_buffer; 

} S_StudentRenderer;


/******************************************************************************
 * Nasledujici fce budete nejspis muset re-implementovat podle sveho
 */

/* Funkce vytvori vas renderer a nainicializuje jej */
S_Renderer * studrenCreate();

/* Funkce korektne zrusi renderer a uvolni pamet */
void studrenRelease(S_Renderer **ppRenderer);

/* Inicializuje frame buffer, depth buffer, apod. */
void studrenCreateBuffers(S_Renderer *pRenderer, int width, int height);

/* Funkce vymaze frame buffer, depth buffer, apod. */
void studrenClearBuffers(S_Renderer *pRenderer);

/* Vykresli i-ty trojuhelnik modelu
 * Pred vykreslenim aplikuje na vrcholy modelu aktualne nastavene
 * transformacni matice!
 * i - index trojuhelniku */
void studrenProjectTriangle(S_Renderer *pRenderer, S_Model *pModel, int i);

/* Rasterizace trojuhelniku */
void studrenDrawTriangle(S_StudentRenderer *pStudRenderer,
                          int x1, int y1, double z1,
                          int x2, int y2, double z2,
                          int x3, int y3, double z3,
                          S_RGBA aa_color, S_RGBA bb_color, S_RGBA cc_color);

#ifdef __cplusplus
}
#endif

#endif /* Student_H */

/*****************************************************************************/
/*****************************************************************************/

