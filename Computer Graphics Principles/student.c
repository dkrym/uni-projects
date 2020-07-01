/*
 * Upravil: David Krym
 */

/******************************************************************************
 * Projekt - Zaklady pocitacove grafiky - IZG
 * spanel@fit.vutbr.cz
 *
 * $Id: student.c 48 2011-02-09 12:59:08Z spanel $
 *
 * Opravy a modifikace:
 * -
 */

#include "student.h"
#include "transform.h"

#include <memory.h>
// kvuli max hodnote double
#include <float.h>

/*****************************************************************************/

S_Renderer * studrenCreate()
{
  S_StudentRenderer * renderer = (S_StudentRenderer *)malloc(sizeof(S_StudentRenderer));
  IZG_CHECK(renderer, "Cannot allocate enough memory");

  /* inicializace default rendereru */
  renInit(&renderer->base);

  /* nastaveni ukazatelu na vase upravene funkce */
  renderer->base.releaseFunc         = studrenRelease;
  renderer->base.createBuffersFunc   = studrenCreateBuffers;
  renderer->base.clearBuffersFunc    = studrenClearBuffers;
  renderer->base.projectTriangleFunc = studrenProjectTriangle;

  /* inicializace nove pridanych casti */
  // inicializace z-bufferu
  renderer->z_buffer = NULL;

  return (S_Renderer *)renderer;
}

/*****************************************************************************/

void studrenRelease(S_Renderer **ppRenderer)
{
  /* ziskame ukazatel na nas typ rendereru */
  S_StudentRenderer ** ppStudRenderer = (S_StudentRenderer **)ppRenderer;

  if( ppStudRenderer && *ppStudRenderer )
  {
    /* dealokace frame bufferu */
    if( (*ppStudRenderer)->base.frame_buffer )
      free((*ppStudRenderer)->base.frame_buffer);

    /* dealokace z-bufferu */
    free((*ppStudRenderer)->z_buffer); 

    free(*ppStudRenderer);
    *ppStudRenderer = NULL;
  }
}

/*****************************************************************************/

void studrenCreateBuffers(S_Renderer *pRenderer, int width, int height)
{
  size_t size;
  unsigned int i, i_end; 
  double *zbuffer;
  S_RGBA * new_frame_buffer;

  /* ziskame ukazatel na nas typ rendereru */
  S_StudentRenderer * pStudRenderer = (S_StudentRenderer *)pRenderer;

  IZG_ASSERT(pRenderer && width >= 0 && height >= 0);

  /* alokace pameti z-bufferu a frame bufferu */
  size = width * height * sizeof(double); // nova velikost
  zbuffer = (double *)realloc(pStudRenderer->z_buffer, size);
  IZG_CHECK(zbuffer, "Cannot allocate frame buffer");
  new_frame_buffer = (S_RGBA *)realloc(pRenderer->frame_buffer, size);
  IZG_CHECK(new_frame_buffer, "Cannot allocate frame buffer");

  /* nastaveni nove velikosti frame bufferu, z-buffer */
  pStudRenderer->z_buffer = zbuffer;
  pRenderer->frame_buffer = new_frame_buffer;
  pRenderer->frame_w = width;
  pRenderer->frame_h = height;

  /* nastaveni max hodnoty z-buffer, vymazani frame bufferu */
  studrenClearBuffers(pRenderer);

  return;
}

/*****************************************************************************/

void studrenClearBuffers(S_Renderer *pRenderer)
{
  size_t size;
  unsigned int i, i_end;

  /* ziskame ukazatel na nas typ rendereru */
  S_StudentRenderer * pStudRenderer = (S_StudentRenderer *)pRenderer;

  IZG_ASSERT(pRenderer);

  /* vsechny pixely cerne, tedy 0 */
  size = pRenderer->frame_w * pRenderer->frame_h * sizeof(S_RGBA);
  memset(pRenderer->frame_buffer, 0, size);

  /* nastaveni max hodnoty z-buffer */
  i_end = pRenderer->frame_w * pRenderer->frame_h;
  for (i = 0; i < i_end; i++)
    pStudRenderer->z_buffer[i] = DBL_MAX; // max hodnota

  return;
}

/*****************************************************************************/

void studrenProjectTriangle(S_Renderer *pRenderer, S_Model *pModel, int i)
{
  /* ziskame ukazatel na nas typ rendereru */
  S_StudentRenderer * pStudRenderer = (S_StudentRenderer *)pRenderer;

  /* Doplneno */
  /* souradnice vrcholu a normala troj. po transformaci */
  S_Coords    aa, bb, cc, nn;
  /* souradnice vrcholu po projekci do roviny obrazovky */     
  int         u1, v1, u2, v2, u3, v3;
  /* kreslici barvy */
  S_RGBA      aa_color, bb_color, cc_color;
  S_Triangle  * triangle;

  IZG_ASSERT(pRenderer && pModel && 
            (i >= 0) && (i < trivecSize(pModel->triangles)));

  /* z modelu si vytahneme trojuhelnik */
  triangle = trivecGetPtr(pModel->triangles, i);

  /* transformace vrcholu matici model */
  trTransformVertex(&aa, cvecGetPtr(pModel->vertices, triangle->v[0]));
  trTransformVertex(&bb, cvecGetPtr(pModel->vertices, triangle->v[1]));
  trTransformVertex(&cc, cvecGetPtr(pModel->vertices, triangle->v[2]));

  /* promitneme vrcholy trojuhelniku na obrazovku */
  trProjectVertex(&u1, &v1, &aa);
  trProjectVertex(&u2, &v2, &bb);
  trProjectVertex(&u3, &v3, &cc);

  /* pro test privracenych troj. a osvetlovaci model
     transformujeme take normalu trojuhelniku */
  trTransformVector(&nn, cvecGetPtr(pModel->trinormals, triangle->n));
  coordsNormalize(&nn);

  /* je troj. privraceny ke kamere, tudiz viditelny? */
  if( !renCalcVisibility(pRenderer, &aa, &nn) )
    return; /* odvracene troj. vubec nekreslime */

  /* Lambertuv osvetlovaci model s konstantnim stinovanim (flat shading)
     vyhodnotime ve vsech vrcholech */
  trTransformVector(&nn, cvecGetPtr(pModel->normals, triangle->v[0]));
  coordsNormalize(&nn);
  aa_color = renCalcLighting(pRenderer, &aa, &nn); // osvetleni
  
  trTransformVector(&nn, cvecGetPtr(pModel->normals, triangle->v[1]));
  coordsNormalize(&nn);
  bb_color = renCalcLighting(pRenderer, &bb, &nn);
  
  trTransformVector(&nn, cvecGetPtr(pModel->normals, triangle->v[2]));
  coordsNormalize(&nn);
  cc_color = renCalcLighting(pRenderer, &cc, &nn);

  /* rasterizace celeho trojuhelniku */
  studrenDrawTriangle(pStudRenderer, u1, v1, aa.z, u2, v2, bb.z,
                      u3, v3, cc.z, aa_color, bb_color, cc_color);

  return;
}

/*****************************************************************************/

/*****************************************************************************/
void studrenDrawTriangle(S_StudentRenderer *pStudRenderer,
                          int x1, int y1, double z1,
                          int x2, int y2, double z2,
                          int x3, int y3, double z3,
                          S_RGBA aa_color, S_RGBA bb_color, S_RGBA cc_color)
{
  int     minx, miny, maxx, maxy;
  int     a1, a2, a3, b1, b2, b3, c1, c2, c3;
  double  lambda1, lambda2, lambda3, z, divider;
  int     s1, s2, s3;
  int     x, y, e1, e2, e3;
  S_RGBA  color;

  S_Renderer * pRenderer = &(pStudRenderer->base);

  IZG_ASSERT(pStudRenderer);

  /* obalka trojuhleniku */
  minx = MIN(x1, MIN(x2, x3));
  maxx = MAX(x1, MAX(x2, x3));
  miny = MIN(y1, MIN(y2, y3));
  maxy = MAX(y1, MAX(y2, y3));

  /* oriznuti podle rozmeru okna */
  miny = MAX(miny, 0);
  maxy = MIN(maxy, pRenderer->frame_h - 1);
  minx = MAX(minx, 0);
  maxx = MIN(maxx, pRenderer->frame_w - 1);

  /* Pineduv alg. rasterizace troj.
     hranova fce je obecna rovnice primky Ax + By + C = 0
     primku prochazejici body (x1, y1) a (x2, y2) urcime jako
     (y1 - y2)x + (x2 - x1)y + x1y2 - x2y1 = 0 */

  /* normala primek - vektor kolmy k vektoru mezi dvema vrcholy, tedy (-dy, dx) */
  a1 = y1 - y2;
  a2 = y2 - y3;
  a3 = y3 - y1;
  b1 = x2 - x1;
  b2 = x3 - x2;
  b3 = x1 - x3;

  /* koeficient C */
  c1 = x1 * y2 - x2 * y1;
  c2 = x2 * y3 - x3 * y2;
  c3 = x3 * y1 - x1 * y3;

  /* vypocet hranove fce (vzdalenost od primky) pro protejsi body */
  s1 = a1 * x3 + b1 * y3 + c1;
  s2 = a2 * x1 + b2 * y1 + c2;
  s3 = a3 * x2 + b3 * y2 + c3;

  /* normalizace, aby vzdalenost od primky byla kladna uvnitr trojuhelniku */
  if( s1 < 0 )
  {
    a1 *= -1;
    b1 *= -1;
    c1 *= -1;
  }
  if( s2 < 0 )
  {
    a2 *= -1;
    b2 *= -1;
    c2 *= -1;
  }
  if( s3 < 0 )
  {
    a3 *= -1;
    b3 *= -1;
    c3 *= -1;
  }

  /* vyplnovani... */
  for( y = miny; y <= maxy; ++y )
  {
    /* inicilizace hranove fce v bode (minx, y) */
    e1 = a1 * minx + b1 * y + c1;
    e2 = a2 * minx + b2 * y + c2;
    e3 = a3 * minx + b3 * y + c3;

    for( x = minx; x <= maxx; ++x )
    {
      if( e1 >= 0 && e2 >= 0 && e3 >= 0 )
      {
        /* barycentricke souradnice */
        lambda1 = (double)((y2-y3)*(x-x3) + (x3-x2)*(y-y3)) / ((y2-y3)*(x1-x3) + (x3-x2)*(y1-y3));
        lambda2 = (double)((y3-y1)*(x-x3) + (x1-x3)*(y-y3)) / ((y3-y1)*(x2-x3) + (x1-x3)*(y2-y3));
        lambda3 = 1.0 - lambda1 - lambda2;

        z = z1*lambda1 + z2*lambda2 + z3*lambda3;

        /* barvy */
        color.red = aa_color.red*lambda1 + bb_color.red*lambda2 + cc_color.red*lambda3;
        color.blue = aa_color.blue*lambda1 + bb_color.blue*lambda2 + cc_color.blue*lambda3;
        color.green = aa_color.green*lambda1 + bb_color.green*lambda2 + cc_color.green*lambda3;
        color.alpha = aa_color.alpha*lambda1 + bb_color.alpha*lambda2 + cc_color.alpha*lambda3;

        if (z <= pStudRenderer->z_buffer[(y*pRenderer->frame_w) + x])
        { /* vykresleni */
          PIXEL(pRenderer, x, y) = color;
          pStudRenderer->z_buffer[(y*pRenderer->frame_w) + x] = z;
        }

      }
        /* hranova fce o pixel vedle */
        e1 += a1;
        e2 += a2;
        e3 += a3;
    }
  }
}
