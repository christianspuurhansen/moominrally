#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <tuple>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_net.h>
#include <pthread.h>
#include <dirent.h>

using namespace std;

#define WINDOWDEAPTH 24
#define GAMEPORT 1234
size_t bredde=1024;
size_t hoejde=768;
bool fuldskaerm=true;
float maks_afstand=150.0;
float min_afstand=0.25;
float friktion=0.992;
float friktion_fri=0.9995;
float tyngdekraft=0.02;
float gear_faktor=0.43;  
float frem_faktor=0.02;

const map<size_t,string> ingen_teksturer;
map<size_t,string> kort_teksturer;
// Matematiske grundfunktioner
inline float sqr(float x) // {{{
{ return x*x;
} // }}}
void cross(float x1, float y1, float z1, float x2, float y2, float z2, float &x, float &y, float &z) // {{{
{ x=y1*z2-z1*y2;
  y=-x1*z2+z1*x2;
  z=x1*y2-y1*x2;
} // }}}
float dot(float x1, float y1, float z1, float x2, float y2, float z2) // {{{
{ return x1*x2+y1*y2+z1*z2;
} // }}}
float dist(float x1, float y1, float z1, float x2, float y2, float z2) // {{{
{ return sqrt(sqr(x2-x1)+sqr(y2-y1)+sqr(z2-z1));
} // }}}

// Roter punkter
inline void roter2d(float x0, float y0, float ang_cos, float ang_sin, float &x1, float &y1) // {{{
{ x1=ang_cos*x0-ang_sin*y0;
  y1=ang_sin*x0+ang_cos*y0;
} // }}}
inline void roter3d(float x1, float y1, float z1, float h_cos, float h_sin, float v_cos, float v_sin, float t_cos, float t_sin, float &x2, float &y2, float &z2) // {{{
{ // Roter om x-aksen
  float x_1=x1;
  float y_1, z_1;
  roter2d(y1,z1,t_cos,t_sin,y_1,z_1);
  // Roter om y-aksen
  float x_2,z_2;
  roter2d(x_1,z_1,v_cos,v_sin,x_2,z_2);
  float y_2=y_1;
  // Roter om z-aksen
  roter2d(x_2,y_2,h_cos,h_sin,x2,y2);
  z2=z_2;
} // }}}
inline void retor3d(float x1, float y1, float z1, float h_cos, float h_sin, float v_cos, float v_sin, float t_cos, float t_sin, float &x2, float &y2, float &z2) // {{{
{ // Roter om y-aksen
  float x_1,z_1;
  roter2d(x1,z1,v_cos,v_sin,x_1,z_1);
  float y_1=y1;
  // Roter om x-aksen
  float x_2=x_1;
  float y_2, z_2;
  roter2d(y_1,z_1,t_cos,t_sin,y_2,z_2);
  // Roter om z-aksen
  roter2d(x_2,y_2,h_cos,h_sin,x2,y2);
  z2=z_2;
} // }}}
inline void find_rotation(float x1, float y1, float z1, float x2, float y2, float z2, float &h_cos, float &h_sin, float &v_cos, float &v_sin) // {{{
{ float dh=dist(x1,y1,0.0,x2,y2,0.0f);
  if (dh>0)
  { h_cos=(x2-x1)/dh;
    h_sin=(y2-y1)/dh;
    //h=atan2(h_sin,h_cos);
  }
  float dv=dist(0.0,0.0,z1,0.0,dh,z2);
  if (dv>0.0f)
  { v_cos=dh/dv;
    v_sin=(z2-z1)/dv;
    //v=atan2(v_sin,v_cos);
  }
} // }}}

// fade_color beregner hvorledes en
// farve falmer, jo længere væk fra
// spilleren den er
inline float fade(float v, float d) // {{{
{ if (d>=maks_afstand)
    return 0;
  if (d<=min_afstand)
    return v;
  return (v/(1.0+18.0*sqr((d-min_afstand)/(maks_afstand-min_afstand))));
} // }}}
inline unsigned char fade_color(unsigned char col, float d) // {{{
{ return (unsigned char)fade((float)col,d);
} // }}}
inline unsigned char fade_rcolor(unsigned char col, float d) // {{{
{ return (unsigned char)fade((float)(col),d);
} // }}}
inline unsigned char fade_gcolor(unsigned char col, float d) // {{{
{ return (unsigned char)fade((float)(col),d);
} // }}}
inline unsigned char fade_bcolor(unsigned char col, float d) // {{{
{ return (unsigned char)fade((float)(col),d);
} // }}}

/* Definer hvad en trekant er
 * Den har tre koordinater i 3D
 * (minX1,minY1,minZ1), (minX2,minY2,minZ2) og
 * (minX3,minY3,minZ3).
 * Derudover har den en farve angivet som
 * rød, grøn og blå værdier imellem 0 og 255
 * minR, minG og minB.
 */
class Trekant // {{{
{ public:
    Trekant(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, int r, int g, int b);
    virtual ~Trekant();

    float minX1, minY1, minZ1, minX2, minY2, minZ2, minX3, minY3, minZ3;
    int minR, minG, minB;
}; // }}}
Trekant::Trekant(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, int r, int g, int b) // {{{
: minX1(x1)
, minY1(y1)
, minZ1(z1)
, minX2(x2)
, minY2(y2)
, minZ2(z2)
, minX3(x3)
, minY3(y3)
, minZ3(z3)
, minR(r)
, minG(g)
, minB(b)
{ 
} // }}}
Trekant::~Trekant() // {{{
{
} // }}}

class Omraader // {{{
{ public:
    Omraader();
    virtual ~Omraader();

    void SetTrekanter(vector<Trekant> &ts); // Indekser trekanter efter omraade
    vector<Trekant*> &Omraade(int x, int y, int z); // Find trekanter i omraade

    static int Afsnit(int v) { return v/(maks_afstand/40); }

  private:
    int Indeks(int x, int y, int z) // {{{
    { if (x<minMinX || y<minMinY || z<minMinZ || x>minMaxX || y>minMaxY || z>minMaxZ )
        return -1;
      return z-minMinZ+(minMaxZ-minMinZ+1)*(y-minMinY+(minMaxY-minMinY+1)*(x-minMinX));
    } // }}}
    vector<Trekant*> *mineOmraader;
    vector<Trekant*> mitTommeOmraade;
    int minMinX;
    int minMinY;
    int minMinZ;
    int minMaxX;
    int minMaxY;
    int minMaxZ;
}; // }}}
Omraader::Omraader() // {{{
{ mineOmraader=NULL;
  minMinX=0;
  minMinY=0;
  minMinZ=0;
  minMaxX=-1;
  minMaxY=-1;
  minMaxZ=-1;
} // }}}
Omraader::~Omraader() // {{{
{ if (mineOmraader)
    delete [] mineOmraader;
} // }}}
void Omraader::SetTrekanter(vector<Trekant> &ts) // {{{
{ if (mineOmraader)
    delete [] mineOmraader;
  mineOmraader=NULL;

  minMinX=0;
  minMinY=0;
  minMinZ=0;
  minMaxX=0;
  minMaxY=0;
  minMaxZ=0;
  for (auto it=ts.begin(); it!=ts.end(); ++it)
  { if (Afsnit(int(it->minX1))<minMinX)
      minMinX=Afsnit(int(it->minX1));
    if (Afsnit(int(it->minY1))<minMinY)
      minMinY=Afsnit(int(it->minY1));
    if (Afsnit(int(it->minZ1))<minMinZ)
      minMinZ=Afsnit(int(it->minZ1));
    if (Afsnit(int(it->minX1))>minMaxX)
      minMaxX=Afsnit(int(it->minX1));
    if (Afsnit(int(it->minY1))>minMaxY)
      minMaxY=Afsnit(int(it->minY1));
    if (Afsnit(int(it->minZ1))>minMaxZ)
      minMaxZ=Afsnit(int(it->minZ1));
  }
  mineOmraader=new vector<Trekant*>[(minMaxX-minMinX+1)*(minMaxY-minMinY+1)*(minMaxZ-minMinZ+1)];
  // Tilføj trekanter til områder
  for (auto it=ts.begin(); it!=ts.end(); ++it)
  { mineOmraader[Indeks(Afsnit(int(it->minX1)),Afsnit(int(it->minY1)),Afsnit(int(it->minZ1)))].push_back(&(*it));
  }
} // }}}
vector<Trekant*> &Omraader::Omraade(int x, int y, int z) // {{{
{ int i=Indeks(x,y,z);
  if (i<0)
    return mitTommeOmraade;
  
  return mineOmraader[i];
} // }}}

class Tilstand;

/* Definer hvad en ting er
 * Den har en figur af trekanter
 * Den har koordinater i 3d (minX,minY,minZ)
 * Den har en horisontal og vertikal retning (myH,myV)
 * Den har en hastighed (minFart)
 */
class Ting // {{{
{ public:
    Ting();
    Ting(stringstream &ss);
    Ting(const string &figur, float x, float y, float z, float h, float v, float t, float fart);
    virtual ~Ting();

    string minFigur;
    float minX, minY, minZ, minH, minV, minT, minFart;
    float minT_cos, minT_sin, minV_cos, minV_sin, minH_cos, minH_sin;
    bool sendt;

    virtual string Type();
    virtual void Bevaeg(size_t ticks, Tilstand &tilstand);
    virtual bool Forsvind();
    virtual string TilTekst();
    void BeregnRetning()
    { minT_cos=cos(minT);
      minT_sin=sin(minT);
      minV_cos=cos(minV);
      minV_sin=sin(minV);
      minH_cos=cos(minH);
      minH_sin=sin(minH);
    }
}; // }}}
Ting::Ting() // {{{
{ minX=0.0;
  minY=0.0;
  minZ=0.0;
  minH=0.0;
  minV=0.0;
  minT=0.0;
  minFart=0.0;
  BeregnRetning();
  sendt=false;
} // }}}
Ting::Ting(stringstream &ss) // {{{
{ ss>>minFigur;
  ss>>minX;
  ss>>minY;
  ss>>minZ;
  ss>>minH;
  ss>>minV;
  ss>>minT;
  ss>>minFart;
  BeregnRetning();
  sendt=false;
} // }}}
Ting::Ting(const string &figur, float x, float y, float z, float h, float v, float t, float fart) // {{{
{ minFigur=figur;
  minX=x;
  minY=y;
  minZ=z;
  minH=h;
  minV=v;
  minT=t;
  minFart=fart;
  BeregnRetning();
  sendt=false;
} // }}}
Ting::~Ting() // {{{
{
} // }}}
string Ting::Type() // {{{
{ return "ting";
} // }}}
void Ting::Bevaeg(size_t ticks, Tilstand &tilstand) // {{{
{
} // }}}
bool Ting::Forsvind() // {{{
{ return false;
} // }}}
string Ting::TilTekst() // {{{
{ stringstream resultat;
  resultat << minFigur << " " << minX << " " << minY << " " << minZ << " " << minH << " " << minV << " " << minT << " " << minFart;
  return resultat.str();
} // }}}
map<string,vector<Trekant> > FigurBibliotek;
map<string,SDL_Surface*> BilledeBibliotek;
map<string,Mix_Chunk*> LydeBibliotek;

// Definer hjul som en ting
class Hjul : public Ting // {{{
{ public:
    Hjul();
    Hjul(stringstream &ss);
    Hjul(float x, float y, float z, float h, float v, float t, float fart);
    virtual ~Hjul();

    virtual void Bevaeg(size_t ticks, Tilstand &tilstand);
    virtual bool Forsvind();
    virtual string Type();
    virtual string TilTekst();
    float minXhastighed;
    float minYhastighed;
    float minZhastighed;
    bool minKontakt; // Har kontakt med jorden
    void Spin(float ang) // {{{
    { minV-=ang;
      minV=minV-((int)(minV/M_PI))*M_PI;
    } // }}}
    void SetRetning(float h, float t) // {{{
    { minH=h;
      minT=t;
      BeregnRetning();
    } // }}}
}; // }}}

class Avatar : public Ting // {{{
{ public:
    Avatar();
    Avatar(stringstream &ss);
    Avatar(const string &figur, float x, float y, float z, float h, float v, float t, float fart);

    virtual void Frem(size_t ticks)=0;
    virtual void Tilbage(size_t ticks)=0;
    virtual void SetRetning(bool venstre, bool hoejre)=0;
    virtual void Tegn(SDL_Surface *dest, vector<float>&zbuf, Tilstand &tilstand)=0;
    virtual void TilCheckpoint(Tilstand &tilstand)=0;

    bool minSmadret;
    int minSmadretTid;
}; // }}}

Avatar::Avatar() // {{{
: Ting()
{
} // }}}
Avatar::Avatar(stringstream &ss) // {{{
: Ting(ss)
{
} // }}}
Avatar::Avatar(const string &figur, float x, float y, float z, float h, float v, float t, float fart) // {{{
: Ting(figur, x, y, z, h, v, t, fart)
{
} // }}}

// Definer bil som en avatar
class Bil : public Avatar // {{{
{ public:
    Bil();
    Bil(stringstream &ss);
    Bil(float x, float y, float z, float h, float v, float t, float fart);
    virtual ~Bil();

    virtual void Bevaeg(size_t ticks, Tilstand &tilstand);
    virtual bool Forsvind();
    virtual string Type();
    virtual string TilTekst();

    virtual void Frem(size_t ticks);
    virtual void Tilbage(size_t ticks);
    virtual void SetRetning(bool venstre, bool hoejre);
    virtual void Tegn(SDL_Surface *dest, vector<float>&zbuf, Tilstand &tilstand);
    virtual void TilCheckpoint(Tilstand &tilstand);

    Hjul minHjul1;
    Hjul minHjul2;
    Hjul minHjul3;
    Hjul minHjul4;
    int minSkridStart;
    int minAccellererStart;

}; // }}}

// Definer fly som en avatar
class Fly : public Avatar // {{{
{ public:
    Fly();
    Fly(stringstream &ss);
    Fly(float x, float y, float z, float h, float v, float t, float fart);
    virtual ~Fly();

    virtual void Bevaeg(size_t ticks, Tilstand &tilstand);
    virtual bool Forsvind();
    virtual string Type();
    virtual string TilTekst();

    virtual void Frem(size_t ticks);
    virtual void Tilbage(size_t ticks);
    virtual void SetRetning(bool venstre, bool hoejre);
    virtual void Tegn(SDL_Surface *dest, vector<float>&zbuf, Tilstand &tilstand);
    virtual void TilCheckpoint(Tilstand &tilstand);

    // Propel minPropel;
    float minZhastighed;
    bool minKontakt;
}; // }}}

// Definer båd som en avatar
class Baad : public Avatar // {{{
{ public:
    Baad();
    Baad(stringstream &ss);
    Baad(float x, float y, float z, float h, float v, float t, float fart);
    virtual ~Baad();

    virtual void Bevaeg(size_t ticks, Tilstand &tilstand);
    virtual bool Forsvind();
    virtual string Type();
    virtual string TilTekst();

    virtual void Frem(size_t ticks);
    virtual void Tilbage(size_t ticks);
    virtual void SetRetning(bool venstre, bool hoejre);
    virtual void Tegn(SDL_Surface *dest, vector<float>&zbuf, Tilstand &tilstand);
    virtual void TilCheckpoint(Tilstand &tilstand);

    float minXhastighed;
    float minYhastighed;
    float minZhastighed;
    bool minKontakt;
}; // }}}

// Indlæs 3d obj fil til trekants liste
vector<Trekant> laes_obj(const string &fname) // {{{
{ ifstream fin(fname);
  vector<Trekant> result;
  vector<float> vs; // Vertices

  string line;
  int r, g, b;
  while (std::getline(fin, line))
  { if (line.substr(0,2)=="v ") // vertex
    { stringstream ss;
      ss << line.substr(2);
      float x, y, z;
      ss >> x;
      ss >> y;
      ss >> z;
      vs.push_back(x);
      vs.push_back(y);
      vs.push_back(z);
    }
    else if (line.substr(0,13)=="usemtl color_") // material (color)
    { size_t col=std::atoi(line.substr(13).c_str());
      b=col%256;
      col=col/256;
      g=col%256;
      col=col/256;
      r=col%256;
    }
    else if (line.substr(0,2)=="f ") // face (triangle)
    { stringstream ss;
      ss << line.substr(2);
      int v1, v2, v3;
      ss >> v1;
      --v1;
      ss >> v2;
      --v2;
      ss >> v3;
      --v3;

      result.push_back(Trekant(vs[3*v1],vs[3*v1+1],vs[3*v1+2],vs[3*v2],vs[3*v2+1],vs[3*v2+2],vs[3*v3],vs[3*v3+1],vs[3*v3+2],r,g,b));
    }
    //else
    //  cout << "Ignoring obj line: " << line << endl;
  }
  fin.close();
  return result;
} // }}}
// Indlæs 3d stl fil til trekants liste
// Eftersom der ikke er farver i stl, angives
// en farve som bruges til trekanterne
vector<Trekant> laes_stl(const string &fname, char r, char g, char b) // {{{
{ ifstream fin(fname);
  vector<Trekant> result;
  fin.ignore(80);
  size_t count=0;
  fin.read((char*)&count,4);
  for (size_t i=0; i<count; ++i)
  { float x0;
    fin.read((char*)&x0,sizeof(float));
    float y0;
    fin.read((char*)&y0,sizeof(float));
    float z0;
    fin.read((char*)&z0,sizeof(float));
    float x1;
    fin.read((char*)&x1,sizeof(float));
    float y1;
    fin.read((char*)&y1,sizeof(float));
    float z1;
    fin.read((char*)&z1,sizeof(float));
    float x2;
    fin.read((char*)&x2,sizeof(float));
    float y2;
    fin.read((char*)&y2,sizeof(float));
    float z2;
    fin.read((char*)&z2,sizeof(float));
    float x3;
    fin.read((char*)&x3,sizeof(float));
    float y3;
    fin.read((char*)&y3,sizeof(float));
    float z3;
    fin.read((char*)&z3,sizeof(float));
    fin.ignore(2);

    result.push_back(Trekant(x1,y1,z1,x2,y2,z2,x3,y3,z3,r,g,b));
  }
  fin.close();
  return result;
} // }}}

// Opdel store trekanter til mindre trekanter
vector<Trekant> smaa_trekanter(const vector<Trekant> &trekanter, float max_afstand=min_afstand*1.5) // {{{
{ vector<Trekant> lektier=trekanter;
  vector<Trekant> resultat;

  while (!lektier.empty())
  { Trekant t=lektier.back();
    lektier.pop_back();
    float d12=sqr(t.minX1-t.minX2)+sqr(t.minY1-t.minY2)+sqr(t.minZ1-t.minZ2);
    float d13=sqr(t.minX1-t.minX3)+sqr(t.minY1-t.minY3)+sqr(t.minZ1-t.minZ3);
    float d23=sqr(t.minX2-t.minX3)+sqr(t.minY2-t.minY3)+sqr(t.minZ2-t.minZ3);
    if (d12>max_afstand && d12>=d13 && d12>=d23) // Opdel linjen p1-p2 {{{
    { lektier.push_back(Trekant(t.minX1, t.minY1, t.minZ1,
                                (t.minX1+t.minX2)/2.0, (t.minY1+t.minY2)/2.0, (t.minZ1+t.minZ2)/2.0,
                                t.minX3, t.minY3, t.minZ3,
                                t.minR, t.minG, t.minB));
      lektier.push_back(Trekant((t.minX1+t.minX2)/2.0, (t.minY1+t.minY2)/2.0, (t.minZ1+t.minZ2)/2.0,
                                t.minX2, t.minY2, t.minZ2,
                                t.minX3, t.minY3, t.minZ3,
                                t.minR, t.minG, t.minB));
    } // }}}
    else if (d13>max_afstand && d13>=d12 && d13>=d23) // Opdel linjen p1-p3 {{{
    { lektier.push_back(Trekant(t.minX1, t.minY1, t.minZ1,
                                t.minX2, t.minY2, t.minZ2,
                                (t.minX1+t.minX3)/2.0, (t.minY1+t.minY3)/2.0, (t.minZ1+t.minZ3)/2.0,
                                t.minR, t.minG, t.minB));
      lektier.push_back(Trekant((t.minX1+t.minX3)/2.0, (t.minY1+t.minY3)/2.0, (t.minZ1+t.minZ3)/2.0,
                                t.minX2, t.minY2, t.minZ2,
                                t.minX3, t.minY3, t.minZ3,
                                t.minR, t.minG, t.minB));
    } // }}}
    else if (d23>max_afstand && d23>=d12 && d23>=d13) // Opdel linjen p2-p3 {{{
    { lektier.push_back(Trekant(t.minX1, t.minY1, t.minZ1,
                                t.minX2, t.minY2, t.minZ2,
                                (t.minX2+t.minX3)/2.0, (t.minY2+t.minY3)/2.0, (t.minZ2+t.minZ3)/2.0,
                                t.minR, t.minG, t.minB));
      lektier.push_back(Trekant(t.minX1, t.minY1, t.minZ1,
                                (t.minX2+t.minX3)/2.0, (t.minY2+t.minY3)/2.0, (t.minZ2+t.minZ3)/2.0,
                                t.minX3, t.minY3, t.minZ3,
                                t.minR, t.minG, t.minB));
    } // }}}
    else // Trekanten er lille nok
      resultat.push_back(t);
  }

  return resultat;
} // }}}

// Skaler objekt, så det fylder præcis det angivne område
vector<Trekant> skaler_ting(float minx, float miny, float minz, float maxx, float maxy, float maxz, const vector<Trekant> &trekanter) // {{{
{ vector<Trekant> resultat;
  if (trekanter.empty())
    return resultat;
  float cminx=trekanter[0].minX1, cminy=trekanter[0].minY1, cminz=trekanter[0].minZ1,
        cmaxx=trekanter[0].minX1, cmaxy=trekanter[0].minY1, cmaxz=trekanter[0].minZ1;
  for (size_t i=0; i<trekanter.size(); ++i)
  { cminx=min(cminx,trekanter[i].minX1);
    cminx=min(cminx,trekanter[i].minX2);
    cminx=min(cminx,trekanter[i].minX3);
    cmaxx=max(cmaxx,trekanter[i].minX1);
    cmaxx=max(cmaxx,trekanter[i].minX2);
    cmaxx=max(cmaxx,trekanter[i].minX3);
    cminy=min(cminy,trekanter[i].minY1);
    cminy=min(cminy,trekanter[i].minY2);
    cminy=min(cminy,trekanter[i].minY3);
    cmaxy=max(cmaxy,trekanter[i].minY1);
    cmaxy=max(cmaxy,trekanter[i].minY2);
    cmaxy=max(cmaxy,trekanter[i].minY3);
    cminz=min(cminz,trekanter[i].minZ1);
    cminz=min(cminz,trekanter[i].minZ2);
    cminz=min(cminz,trekanter[i].minZ3);
    cmaxz=max(cmaxz,trekanter[i].minZ1);
    cmaxz=max(cmaxz,trekanter[i].minZ2);
    cmaxz=max(cmaxz,trekanter[i].minZ3);
  }
  if (cminx==cmaxx || cminy==cmaxy || cminz==cmaxz)
    return resultat;
  float skalerx=cmaxx==cminx?1.0:(maxx-minx)/(cmaxx-cminx);
  float skalery=cmaxy==cminy?1.0:(maxy-miny)/(cmaxy-cminy);
  float skalerz=cmaxz==cminz?1.0:(maxz-minz)/(cmaxz-cminz);
  float flytx=minx-cminx*skalerx;
  float flyty=miny-cminy*skalery;
  float flytz=minz-cminz*skalerz;
  for (size_t i=0; i<trekanter.size(); ++i)
  { Trekant t=trekanter[i];
    t.minX1=flytx+t.minX1*skalerx;
    t.minX2=flytx+t.minX2*skalerx;
    t.minX3=flytx+t.minX3*skalerx;
    t.minY1=flyty+t.minY1*skalery;
    t.minY2=flyty+t.minY2*skalery;
    t.minY3=flyty+t.minY3*skalery;
    t.minZ1=flytz+t.minZ1*skalerz;
    t.minZ2=flytz+t.minZ2*skalerz;
    t.minZ3=flytz+t.minZ3*skalerz;
    resultat.push_back(t);
  }

  return resultat;
} // }}}

/* Definer hvad en tilstand er.
 * I en tilstand gemmes de informationer,
 * der beskriver spillets tilstand.
 * Det inkluderer bilen og kameraets position
 * og horisontale og vertikale retning (mig_h og mig_v).
 * Desuden gemmes hvilke spillertaster,
 * der er trykket ned, og om spillet er
 * afsluttet.
 */
class Tilstand // {{{
{ public:
    Tilstand();
    virtual ~Tilstand();
    // Taster
    bool tast_venstre;
    bool tast_hoejre;
    bool tast_frem;
    bool tast_bak;
    bool tast_mellemrum;
    // Bane
    vector<Trekant> bane;
    Omraader omraader;
    // Objekter
    vector<Ting*> ting;
    // Bilen
    Avatar *mig;
    // Kameraets position (retning er altid imod bilen)
    float kamera_x;
    float kamera_y;
    float kamera_z;
    float kamera_h_cos;
    float kamera_h_sin;
    float kamera_v_cos;
    float kamera_v_sin;
    //Checkpoint
    float checkpoint_x;
    float checkpoint_y;
    float checkpoint_z;
    bool tegn_stereogram;

    string banesti;
    bool bane_faerdig;
    pthread_mutex_t klienter_laas;
    bool quit;
}; // }}}
Tilstand::Tilstand() // {{{
: tast_venstre(false)
, tast_hoejre(false)
, tast_frem(false)
, tast_bak(false)
, tast_mellemrum(false)
, kamera_x(0.0)
, kamera_y(-3.0)
, kamera_z(12.0)
, kamera_h_cos(0.0)
, kamera_h_sin(1.0)
, kamera_v_cos(1.0)
, kamera_v_sin(0.0)
, checkpoint_x(0.0)
, checkpoint_y(0.0)
, checkpoint_z(10.0)
, tegn_stereogram(false)
, bane_faerdig(false)
, quit(false)
{ pthread_mutex_init(&klienter_laas,NULL);
  mig=new Fly(0.0, 0.0, 10.0, 0.0, 0.0, 0.0, 10.0);
} // }}}
Tilstand::~Tilstand() // {{{
{ while (!ting.empty())
  { delete ting.back();
    ting.pop_back();
  }
} // }}}

void indlaes_bane(const string &fname, Tilstand &tilstand) // {{{
{ // Åben 3D-Model
  string ext=fname.substr(fname.length()-4);
  if (ext==".stl")
    tilstand.bane=laes_stl(fname,0,0,255);
  else if (ext==".obj")
    tilstand.bane=laes_obj(fname);

  cout << "Verdenen indeholder " << tilstand.bane.size() << " trekanter." << endl;
  tilstand.bane=smaa_trekanter(tilstand.bane);
  cout << "Verdenen indeholder " << tilstand.bane.size() << " små trekanter." << endl;

  // Indekser banens trekanter efter område
  tilstand.omraader.SetTrekanter(tilstand.bane);
} // }}}

// Læs en pixel fra tekstur
Uint32 getpixel(SDL_Surface *surface, int x, int y) // {{{
{
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to retrieve */
  Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

  switch (bpp)
  {
    case 1:
      return *p;
      break;

    case 2:
      return *(Uint16 *)p;
      break;

    case 3:
      if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        return p[0] << 16 | p[1] << 8 | p[2];
      else
        return p[0] | p[1] << 8 | p[2] << 16;
      break;

    case 4:
      return *(Uint32 *)p;
      break;

    default:
      return 0;       /* shouldn't happen, but avoids warnings */
  }
} // }}}

// Tegn en horisontal linje på dest
// De skærmkoordinater der tegnes på,
// gemmes afstanden i zbuf, så det altid
// er det nærmeste objekt der kan ses
inline void tegn_hline(SDL_Surface *dest, vector<float> &zbuf, int y, int x1, float d1, int x2,float d2, unsigned char r, unsigned char g, unsigned char b) // {{{
{ //cout << "tegn_hline(" << y << "," << x1 << "," << d1 << "," << x2 << "," << d2 << "," << r << "," << g << "," << b <<")" << endl;

  if (x2<x1)
    return tegn_hline(dest,zbuf,y,x2,d2,x1,d1,r,g,b);

  if (y<0)
    return;
  if (y>=int(hoejde))
    return;
  if (x2<0)
    return;
  if (x1>=int(bredde))
    return;

  if (x1<0)
  { d1=d1-(d2-d1)*float(x1)/(float(x2)-float(x1));
    x1=0;
  }
  if (x2>bredde)
  { d2=d2+(d1-d2)*(float(x2)-float(bredde))/(float(x2)-float(x1));
    x2=bredde;
  }
  if (d1<=min_afstand && d2<=min_afstand)
    return;
  if (d1>=maks_afstand && d2>=maks_afstand)
    return;
    
  if (x2>x1)
  { float trind=(d2-d1)/(x2-x1);
    float posd=d1;
    for (size_t x=x1; x<x2; ++x)
    { if (posd>min_afstand && posd<zbuf[x+y*bredde])
      { //pixelRGBA(dest,x,y,r,g,b,255);
        pixelRGBA(dest,x,y,fade_rcolor(r,posd),fade_gcolor(g,posd),fade_bcolor(b,posd),255);
        zbuf[x+y*bredde]=posd;
      }
      posd+=trind;
    }
  }
} // }}}
inline void tegn_hline_tekstur(SDL_Surface *dest, vector<float> &zbuf, int y, int x1, float d1, int x2,float d2, SDL_Surface *tekstur, float tx1, float ty1, float tx2, float ty2) // {{{
{ //cout << "tegn_hline(" << y << "," << x1 << "," << d1 << "," << x2 << "," << d2 << "," << r << "," << g << "," << b <<")" << endl;

  if (x2<x1)
    return tegn_hline_tekstur(dest,zbuf,y,x2,d2,x1,d1,tekstur,tx2,ty2,tx1,ty1);

  if (y<0)
    return;
  if (y>=int(hoejde))
    return;
  if (x2<0)
    return;
  if (x1>=int(bredde))
    return;

  if (x1<0)
  { d1=d1-(d2-d1)*float(x1)/(float(x2)-float(x1));
    tx1=tx1-(tx2-tx1)*float(x1)/(float(x2)-float(x1));
    ty1=ty1-(ty2-ty1)*float(x1)/(float(x2)-float(x1));
    x1=0;
  }
  if (x2>bredde)
  { d2=d2+(d1-d2)*(float(x2)-float(bredde))/(float(x2)-float(x1));
    tx2=tx2+(tx1-tx2)*(float(x2)-float(bredde))/(float(x2)-float(x1));
    ty2=ty2+(ty1-ty2)*(float(x2)-float(bredde))/(float(x2)-float(x1));
    x2=bredde;
  }
  if (d1<=min_afstand && d2<=min_afstand)
    return;
  if (d1>=maks_afstand && d2>=maks_afstand)
    return;
    
  if (x2>x1)
  { float trind=(d2-d1)/(x2-x1);
    float posd=d1;
    float trintx=(tx2-tx1)/(x2-x1);
    float trinty=(ty2-ty1)/(x2-x1);
    float tx=tx1;
    float ty=ty1;
    for (size_t x=x1; x<x2; ++x)
    { if (posd>min_afstand && posd<zbuf[x+y*bredde])
      { size_t txi=((((int)(tx*50.0))%tekstur->w)+tekstur->w)%tekstur->w;
        size_t tyi=((((int)(ty*50.0))%tekstur->h)+tekstur->h)%tekstur->h;
        //cout << "txi,tyi: " << txi << "," << tyi << endl;
        Uint32 pixel = getpixel(tekstur, txi, tyi);
        unsigned char r,g,b,a;
        SDL_GetRGBA(pixel, tekstur->format, &r, &g, &b, &a);
        pixelRGBA(dest,x,y,fade_rcolor(r,posd),fade_gcolor(g,posd),fade_bcolor(b,posd),a);
        //pixelRGBA(dest,x,y,fade_rcolor(txi%256,posd),fade_gcolor(tyi%255,posd),fade_bcolor(0,posd),255);
        zbuf[x+y*bredde]=posd;
      }
      posd+=trind;
      tx+=trintx;
      ty+=trinty;
    }
  }
} // }}}

// Tegn en trekant på dest ud fra skærmkoordinater
inline void tegn_trekant2d(SDL_Surface *dest, vector<float> &zbuf, int x1, int y1, float d1, int x2, int y2, float d2, int x3, int y3, float d3, int r, int g, int b) // {{{
{ // Test om vi kan afvise at tegne
  if (d1<=min_afstand || d2<=min_afstand || d3<=min_afstand)
    return;

  if (x1<0 && x2<0 && x3<0)
    return;

  if (x1>=bredde && x2>=bredde && x3>=bredde)
    return;

  if (y1<0 && y2<0 && y3<0)
    return;

  if (y1>=hoejde && y2>=hoejde && y3>=hoejde)
    return;

  // Sorter hjørner
  if (y2<y1)
    return tegn_trekant2d(dest,zbuf,x2,y2,d2,x1,y1,d1,x3,y3,d3,r,g,b);

  if (y3<y1)
    return tegn_trekant2d(dest,zbuf,x3,y3,d3,x2,y2,d2,x1,y1,d1,r,g,b);

  if (y3<y2)
    return tegn_trekant2d(dest,zbuf,x1,y1,d1,x3,y3,d3,x2,y2,d2,r,g,b);

  // Debug - marker hjørner
  //if (x1>=0 && x1<bredde && y1>=0 && y1<hoejde && d1<=zbuf[x1+y1*bredde])
  //{ pixelRGBA(dest,x1,y1,255,255,255,255);
  //  zbuf[x1+y1*bredde]=d1;
  //}
  //if (x2>=0 && x2<bredde && y2>=0 && y2<hoejde && d2<=zbuf[x2+y2*bredde])
  //{ pixelRGBA(dest,x2,y2,255,255,255,255);
  //  zbuf[x2+y2*bredde]=d2;
  //}
  //if (x3>=0 && x3<bredde && y3>=0 && y3<hoejde && d3<=zbuf[x3+y3*bredde])
  //{ pixelRGBA(dest,x3,y3,255,255,255,255);
  //  zbuf[x3+y3*bredde]=d3;
  //}
  
  //Tegn hver hlinje
  float posx2=x1;
  float posx3=x1;
  float posd2=d1;
  float posd3=d1;
  int posy=y1;
  if (y2>y1) // Tegn ned til y2
  { float trinx2=(float(x2)-float(x1))/(float(y2)-float(y1));
    float trind2=(d2-d1)/(float(y2)-float(y1));
    float trinx3=(float(x3)-float(x1))/(float(y3)-float(y1));
    float trind3=(d3-d1)/(float(y3)-float(y1));
    for (int y=0; y<y2-y1; ++y)
    { tegn_hline(dest,zbuf,posy,int(posx2),posd2,int(posx3),posd3,r,g,b);
      posx2+=trinx2;
      posd2+=trind2;
      posx3+=trinx3;
      posd3+=trind3;
      posy+=1;
    }
  }
  posx2=x2;
  posd2=d2;
  if (y3>y2) // Tegn ned til y3
  { float trinx2=(float(x3)-float(x2))/(float(y3)-float(y2));
    float trind2=(d3-d2)/(float(y3)-float(y2));
    float trinx3=(float(x3)-float(x1))/(float(y3)-float(y1));
    float trind3=(d3-d1)/(float(y3)-float(y1));
    for (int y=0; y<y3-y2; ++y)
    { tegn_hline(dest,zbuf,posy,int(posx2),posd2,int(posx3),posd3,r,g,b);
      posx2+=trinx2;
      posd2+=trind2;
      posx3+=trinx3;
      posd3+=trind3;
      posy+=1;
    }
  }
} // }}}
inline void tegn_trekant2d_tekstur(SDL_Surface *dest, vector<float> &zbuf, int x1, int y1, float d1, int x2, int y2, float d2, int x3, int y3, float d3, SDL_Surface *tekstur, float tx1, float ty1, float tx2, float ty2, float tx3, float ty3) // {{{
{ // Test om vi kan afvise at tegne
  if (d1<=min_afstand || d2<=min_afstand || d3<=min_afstand)
    return;

  if (x1<0 && x2<0 && x3<0)
    return;

  if (x1>=bredde && x2>=bredde && x3>=bredde)
    return;

  if (y1<0 && y2<0 && y3<0)
    return;

  if (y1>=hoejde && y2>=hoejde && y3>=hoejde)
    return;

  // Sorter hjørner
  if (y2<y1)
    return tegn_trekant2d_tekstur(dest,zbuf,x2,y2,d2,x1,y1,d1,x3,y3,d3,tekstur,tx2,ty2,tx1,ty1,tx3,ty3);

  if (y3<y1)
    return tegn_trekant2d_tekstur(dest,zbuf,x3,y3,d3,x2,y2,d2,x1,y1,d1,tekstur,tx3,ty3,tx2,ty2,tx1,ty1);

  if (y3<y2)
    return tegn_trekant2d_tekstur(dest,zbuf,x1,y1,d1,x3,y3,d3,x2,y2,d2,tekstur,tx1,ty1,tx3,ty3,tx2,ty2);

  // Debug - marker hjørner
  //if (x1>=0 && x1<bredde && y1>=0 && y1<hoejde && d1<=zbuf[x1+y1*bredde])
  //{ pixelRGBA(dest,x1,y1,255,255,255,255);
  //  zbuf[x1+y1*bredde]=d1;
  //}
  //if (x2>=0 && x2<bredde && y2>=0 && y2<hoejde && d2<=zbuf[x2+y2*bredde])
  //{ pixelRGBA(dest,x2,y2,255,255,255,255);
  //  zbuf[x2+y2*bredde]=d2;
  //}
  //if (x3>=0 && x3<bredde && y3>=0 && y3<hoejde && d3<=zbuf[x3+y3*bredde])
  //{ pixelRGBA(dest,x3,y3,255,255,255,255);
  //  zbuf[x3+y3*bredde]=d3;
  //}
  
  //Tegn hver hlinje
  float posx2=x1;
  float posx3=x1;
  float posd2=d1;
  float posd3=d1;
  float postx2=tx1;
  float posty2=ty1;
  float postx3=tx1;
  float posty3=ty1;
  int posy=y1;
  if (y2>y1) // Tegn ned til y2
  { float trinx2=(float(x2)-float(x1))/(float(y2)-float(y1));
    float trind2=(d2-d1)/(float(y2)-float(y1));
    float trinx3=(float(x3)-float(x1))/(float(y3)-float(y1));
    float trind3=(d3-d1)/(float(y3)-float(y1));
    float trintx2=(tx2-tx1)/(float(y2)-float(y1));
    float trinty2=(ty2-ty1)/(float(y2)-float(y1));
    float trintx3=(tx3-tx1)/(float(y3)-float(y1));
    float trinty3=(ty3-ty1)/(float(y3)-float(y1));
    for (int y=0; y<y2-y1; ++y)
    { tegn_hline_tekstur(dest,zbuf,posy,int(posx2),posd2,int(posx3),posd3,tekstur,postx2,posty2,postx3,posty3);
      posx2+=trinx2;
      posd2+=trind2;
      posx3+=trinx3;
      posd3+=trind3;
      postx2+=trintx2;
      posty2+=trinty2;
      postx3+=trintx3;
      posty3+=trinty3;
      posy+=1;
    }
  }
  posx2=x2;
  posd2=d2;
  postx2=tx2;
  posty2=ty2;
  if (y3>y2) // Tegn ned til y3
  { float trinx2=(float(x3)-float(x2))/(float(y3)-float(y2));
    float trind2=(d3-d2)/(float(y3)-float(y2));
    float trinx3=(float(x3)-float(x1))/(float(y3)-float(y1));
    float trind3=(d3-d1)/(float(y3)-float(y1));
    float trintx2=(tx3-tx2)/(float(y3)-float(y2));
    float trinty2=(ty3-ty2)/(float(y3)-float(y2));
    float trintx3=(tx3-tx1)/(float(y3)-float(y1));
    float trinty3=(ty3-ty1)/(float(y3)-float(y1));
    for (int y=0; y<y3-y2; ++y)
    { tegn_hline_tekstur(dest,zbuf,posy,int(posx2),posd2,int(posx3),posd3,tekstur,postx2,posty2,postx3,posty3);
      posx2+=trinx2;
      posd2+=trind2;
      posx3+=trinx3;
      posd3+=trind3;
      postx2+=trintx2;
      posty2+=trinty2;
      postx3+=trintx3;
      posty3+=trinty3;
      posy+=1;
    }
  }
} // }}}

// Tegn en trekant ud fra 3D koordinater
inline void tegn_trekant3d(SDL_Surface *dest, vector<float>&zbuf, Tilstand &t, const Trekant &trekant, const map<size_t,string> &teksturer, float tx1=NAN, float ty1=NAN, float tx2=NAN, float ty2=NAN, float tx3=NAN, float ty3=NAN) // {{{
{ // Forskyd koordinater
  float ax1=trekant.minX1-t.kamera_x;
  float ay1=trekant.minY1-t.kamera_y;
  float az1=trekant.minZ1-t.kamera_z;
  float ax2=trekant.minX2-t.kamera_x;
  float ay2=trekant.minY2-t.kamera_y;
  float az2=trekant.minZ2-t.kamera_z;
  float ax3=trekant.minX3-t.kamera_x;
  float ay3=trekant.minY3-t.kamera_y;
  float az3=trekant.minZ3-t.kamera_z;
  // Roter om z-aksen
  float rx1=ax1*t.kamera_h_sin-ay1*t.kamera_h_cos;
  float ry1=ay1*t.kamera_h_sin+ax1*t.kamera_h_cos;
  float rz1=az1;
  float rx2=ax2*t.kamera_h_sin-ay2*t.kamera_h_cos;
  float ry2=ay2*t.kamera_h_sin+ax2*t.kamera_h_cos;
  float rz2=az2;
  float rx3=ax3*t.kamera_h_sin-ay3*t.kamera_h_cos;
  float ry3=ay3*t.kamera_h_sin+ax3*t.kamera_h_cos;
  float rz3=az3;
  // Roter om x-aksen
  float fx1=rx1;
  float fy1=ry1*t.kamera_v_cos+rz1*t.kamera_v_sin;
  float fz1=rz1*t.kamera_v_cos-ry1*t.kamera_v_sin;
  float fx2=rx2;
  float fy2=ry2*t.kamera_v_cos+rz2*t.kamera_v_sin;
  float fz2=rz2*t.kamera_v_cos-ry2*t.kamera_v_sin;
  float fx3=rx3;
  float fy3=ry3*t.kamera_v_cos+rz3*t.kamera_v_sin;
  float fz3=rz3*t.kamera_v_cos-ry3*t.kamera_v_sin;
  // Find skaermkoordinater
  if (fy1<min_afstand || fy2<min_afstand || fy3<min_afstand)
    return;
  int x1=int(0.5*bredde+(fx1/fy1)*bredde);
  int y1=int(0.5*hoejde-(fz1/fy1)*bredde);
  int x2=int(0.5*bredde+(fx2/fy2)*bredde);
  int y2=int(0.5*hoejde-(fz2/fy2)*bredde);
  int x3=int(0.5*bredde+(fx3/fy3)*bredde);
  int y3=int(0.5*hoejde-(fz3/fy3)*bredde);
  map<size_t,string>::const_iterator tekstur=teksturer.find(trekant.minR*256*256+trekant.minG*256+trekant.minB);
  if (tekstur!=teksturer.end())
    tegn_trekant2d_tekstur(dest,zbuf,x1,y1,fy1,x2,y2,fy2,x3,y3,fy3,BilledeBibliotek[tekstur->second],isnan(tx1)?trekant.minX1+trekant.minZ1:tx1,isnan(ty1)?trekant.minY1-trekant.minZ1:ty1,isnan(tx2)?trekant.minX2+trekant.minZ2:tx2,isnan(ty2)?trekant.minY2-trekant.minZ2:ty2,isnan(tx3)?trekant.minX3+trekant.minZ3:tx3,isnan(ty3)?trekant.minY3-trekant.minZ3:ty3);
  else
  //{ cout << "Trekant farve: " << (int)trekant.minR << "," << (int)trekant.minG<< "," << (int)trekant.minB << endl;
    tegn_trekant2d(dest,zbuf,x1,y1,fy1,x2,y2,fy2,x3,y3,fy3,trekant.minR,trekant.minG,trekant.minB);
  //}
} // }}}

// Tegn en ting
enum texture_mode {OBJECT_SPACE=0, WORLD_SPACE=1};
inline void tegn_ting(SDL_Surface *dest, vector<float>&zbuf, Tilstand &tilstand, const Ting &ting, const map<size_t,string> &teksturer, texture_mode tm=OBJECT_SPACE) // {{{
{ // For hver trekant
  for (size_t i=0; i<FigurBibliotek[ting.minFigur].size(); ++i)
  { Trekant t=FigurBibliotek[ting.minFigur][i];
    // Roter
    Trekant t2=t;
    retor3d(t.minX1,t.minY1,t.minZ1,ting.minH_cos,ting.minH_sin,ting.minV_cos,ting.minV_sin,ting.minT_cos,ting.minT_sin,t2.minX1,t2.minY1,t2.minZ1);
    retor3d(t.minX2,t.minY2,t.minZ2,ting.minH_cos,ting.minH_sin,ting.minV_cos,ting.minV_sin,ting.minT_cos,ting.minT_sin,t2.minX2,t2.minY2,t2.minZ2);
    retor3d(t.minX3,t.minY3,t.minZ3,ting.minH_cos,ting.minH_sin,ting.minV_cos,ting.minV_sin,ting.minT_cos,ting.minT_sin,t2.minX3,t2.minY3,t2.minZ3);
    // Flyt
    Trekant t3=t2;
    t3.minX1=t2.minX1+ting.minX;
    t3.minY1=t2.minY1+ting.minY;
    t3.minZ1=t2.minZ1+ting.minZ;
    t3.minX2=t2.minX2+ting.minX;
    t3.minY2=t2.minY2+ting.minY;
    t3.minZ2=t2.minZ2+ting.minZ;
    t3.minX3=t2.minX3+ting.minX;
    t3.minY3=t2.minY3+ting.minY;
    t3.minZ3=t2.minZ3+ting.minZ;
    if (tm==OBJECT_SPACE)
      tegn_trekant3d(dest,zbuf,tilstand,t3,teksturer,t.minX1+t.minZ1,t.minY1-t.minZ1,t.minX2+t.minZ2,t.minY2-t.minZ2,t.minX3+t.minZ3,t.minY3-t.minZ3);
    else
      tegn_trekant3d(dest,zbuf,tilstand,t3,teksturer,t3.minX1+t3.minZ1,t3.minY1-t3.minZ1,t3.minX2+t3.minZ2,t3.minY2-t3.minZ2,t3.minX3+t3.minZ3,t3.minY3-t3.minZ3);
  }
} // }}}

void rammer(Tilstand &tilstand, const Trekant &trekant) // {{{
{ if (trekant.minB>=200 && trekant.minG<=160 && trekant.minR<=160)
  { //cout << "Rammer med farve: " << (int)trekant.minR << "," << (int)trekant.minG << "," << (int)trekant.minB << endl;
    tilstand.bane_faerdig=true;
  }
} // }}}

// Implementer hjul som en ting
Hjul::Hjul() // {{{
: Ting("hjul",0.0,0.0,0.0,0.0,0.0,0.0,0.0)
{ SetRetning(0.0,0.0);
  minXhastighed=0.0;
  minYhastighed=0.0;
  minZhastighed=0.0;
} // }}}
Hjul::Hjul(stringstream &ss) // {{{
: Ting()
{ ss>>minX;
  ss>>minY;
  ss>>minZ;
  ss>>minH;
  ss>>minV;
  ss>>minFart;
  SetRetning(minH,minT);

  ss>>minXhastighed;
  ss>>minYhastighed;
  ss>>minZhastighed;
} // }}}
Hjul::Hjul(float x, float y, float z, float h, float v, float t, float fart) // {{{
: Ting("hjul",x,y,z,h,v,t,fart)
{ minXhastighed=minH_cos*minV_cos*fart;
  minYhastighed=minH_sin*minV_cos*fart;
  minZhastighed=minV_sin*fart;
  SetRetning(minH,minT);
} // }}}
Hjul::~Hjul() // {{{
{
} // }}}
string Hjul::Type() // {{{
{ return "hjul";
} // }}}
void Hjul::Bevaeg(size_t ticks, Tilstand &tilstand) // {{{
{ // Kollisioner
  minKontakt=false;
  for (int x=Omraader::Afsnit(int(minX)-1); x<=Omraader::Afsnit(int(minX)+1); ++x)
  { for (int y=Omraader::Afsnit(int(minY)-1); y<=Omraader::Afsnit(int(minY)+1); ++y)
    { for (int z=Omraader::Afsnit(int(minZ)-1); z<=Omraader::Afsnit(int(minZ)+1); ++z)
      { vector<Trekant*> &omraade(tilstand.omraader.Omraade(x,y,z));
        for (size_t i=0; i<omraade.size(); ++i)
        { const Trekant *t=omraade[i];
          float d1=dist(t->minX1,t->minY1,t->minZ1,minX,minY,minZ);
          if (d1<=0.5) // Hjul radius
          { // Rammer
            rammer(tilstand,*t);
            minKontakt=true;
            float retningX=minX-t->minX1;
            float retningY=minY-t->minY1;
            float retningZ=minZ-t->minZ1;
            if (d1<=0.001f)
            { retningX=0.1f;
              d1=0.1f;
            }
            minX+=(0.5-d1)*retningX; // Correct position
            minY+=(0.5-d1)*retningY; // Correct position
            minZ+=(0.5-d1)*retningZ; // Correct position
            minXhastighed+=retningX*(abs(minXhastighed*1.0)+1.0)*0.001f; // Bounce
            minYhastighed+=retningY*(abs(minYhastighed*1.0)+1.0)*0.001f; // Bounce
            minZhastighed+=retningZ*(abs(minZhastighed*1.0)+1.0)*0.001f; // Bounce
          }
          float d2=dist(t->minX2,t->minY2,t->minZ2,minX,minY,minZ);
          if (d2<=0.5)
          { // Rammer
            rammer(tilstand,*t);
            minKontakt=true;
            float retningX=minX-t->minX2;
            float retningY=minY-t->minY2;
            float retningZ=minZ-t->minZ2;
            if (d2<=0.001f)
            { retningX=0.1f;
              d2=0.1f;
            }
            minX+=(0.5-d2)*retningX; // Correct position
            minY+=(0.5-d2)*retningY; // Correct position
            minZ+=(0.5-d2)*retningZ; // Correct position
            minXhastighed+=retningX*(abs(minXhastighed*1.0)+1.0)*0.001f; // Bounce
            minYhastighed+=retningY*(abs(minYhastighed*1.0)+1.0)*0.001f; // Bounce
            minZhastighed+=retningZ*(abs(minZhastighed*1.0)+1.0)*0.001f; // Bounce
          }
          float d3=dist(t->minX3,t->minY3,t->minZ3,minX,minY,minZ);
          if (d3<=0.5)
          { // Rammer
            rammer(tilstand,*t);
            minKontakt=true;
            float retningX=minX-t->minX3;
            float retningY=minY-t->minY3;
            float retningZ=minZ-t->minZ3;
            if (d3<=0.001f)
            { retningX=0.1f;
              d3=0.1f;
            }
            minX+=(0.5-d3)*retningX; // Correct position
            minY+=(0.5-d3)*retningY; // Correct position
            minZ+=(0.5-d3)*retningZ; // Correct position
            minXhastighed+=retningX*(abs(minXhastighed*1.0)+1.0)*0.001f; // Bounce
            minYhastighed+=retningY*(abs(minYhastighed*1.0)+1.0)*0.001f; // Bounce
            minZhastighed+=retningZ*(abs(minZhastighed*1.0)+1.0)*0.001f; // Bounce
          }
        }
      }
    }
  }
  // Tyngdekraft
  minZhastighed-=tyngdekraft*float(ticks);
  // Friktion
  if (minKontakt)
  { minXhastighed*=friktion;
    minYhastighed*=friktion;
    minZhastighed*=friktion;
    //Mix_PlayChannel( 1, LydeBibliotek["hop"], 0 );
  }
  else
  { minXhastighed*=friktion_fri;
    minYhastighed*=friktion_fri;
    minZhastighed*=friktion_fri;
  }

  // Udfør bevaegelse
  minX+=minXhastighed*0.001*ticks;
  minY+=minYhastighed*0.001*ticks;
  minZ+=minZhastighed*0.001*ticks;
} // }}}
bool Hjul::Forsvind() // {{{
{ return minZ<=-1.0;
} // }}}
string Hjul::TilTekst() // {{{
{ stringstream resultat;
  resultat << minX << " " << minY << " " << minZ << " " << minH << " " << minV << " " << minFart << " "
           << minXhastighed << " " << minYhastighed << " " << minZhastighed;
  return resultat.str();
} // }}}

// Implementer bil som en ting
Bil::Bil() // {{{
: Avatar("bil",0.0,0.0,0.0,0.0,0.0,0.0,0.0)
, minHjul1(2, 1, 0, 0, 0, 0, 0)
, minHjul2(2, -1, 0, 0, 0, 0, 0)
, minHjul3(-2, -1, 0, 0, 0, 0, 0)
, minHjul4(-2, 1, 0, 0, 0, 0, 0)
{ minSmadret=false;
  // Start lyde
  Mix_Volume(5, 0);
  Mix_PlayChannel(5, LydeBibliotek["bil_motor"], -1 );
  Mix_Volume(6, 0);
  Mix_PlayChannel(6, LydeBibliotek["bil_skrid"], -1 );
  minSkridStart=100;
  minAccellererStart=100;
} // }}}
Bil::Bil(stringstream &ss) // {{{
: Avatar()
, minHjul1(ss)
, minHjul2(ss)
, minHjul3(ss)
, minHjul4(ss)
{ ss>>minSmadret;
  ss>>minSmadretTid;
  // Start lyde
  Mix_Volume(5, 0);
  Mix_PlayChannel(5, LydeBibliotek["bil_motor"], -1 );
  Mix_Volume(6, 0);
  Mix_PlayChannel(6, LydeBibliotek["bil_skrid"], -1 );
  minSkridStart=100;
  minAccellererStart=100;
} // }}}
Bil::Bil(float x, float y, float z, float h, float v, float t, float fart) // {{{
: Avatar("bil",x,y,z,h,v,t,fart)
, minHjul1(x+2.0,y+1.0,z,h,v,t,fart)
, minHjul2(x+2.0,y-1.0,z,h,v,t,fart)
, minHjul3(x-2.0,y-1.0,z,h,v,t,fart)
, minHjul4(x-2.0,y+1.0,z,h,v,t,fart)
{ minSmadret=false;
  // Start lyde
  Mix_Volume(5, 0);
  Mix_PlayChannel(5, LydeBibliotek["bil_motor"], -1 );
  Mix_Volume(6, 0);
  Mix_PlayChannel(6, LydeBibliotek["bil_skrid"], -1 );

  minSkridStart=100;
  minAccellererStart=100;
} // }}}
Bil::~Bil() // {{{
{
} // }}}
string Bil::Type() // {{{
{ return "bil";
} // }}}
void Bil::Bevaeg(size_t ticks, Tilstand &tilstand) // {{{
{ // Håndter styrt
  if (minSmadret)
  { if (minSmadretTid>=5000)
    { minSmadret=false;
      minSmadretTid=0;
      TilCheckpoint(tilstand);
    }
    else
      minSmadretTid+=ticks;
  }
  else
  {
    // Bliver bilen ramt
    for (int x=Omraader::Afsnit(int(minX)-1); x<=Omraader::Afsnit(int(minX)+1); ++x)
    { for (int y=Omraader::Afsnit(int(minY)-1); y<=Omraader::Afsnit(int(minY)+1); ++y)
      { for (int z=Omraader::Afsnit(int(minZ)-1); z<=Omraader::Afsnit(int(minZ)+1); ++z)
        { vector<Trekant*> &omraade(tilstand.omraader.Omraade(x,y,z));
          for (size_t i=0; i<omraade.size(); ++i)
          { const Trekant *t=omraade[i];
            float d1=dist(t->minX1,t->minY1,t->minZ1,minX-minV_sin*minH_cos,minY-minV_sin*minH_sin,minZ+minV_cos);
            if (d1<=0.3) // Hjul radius
            { // Rammer
              if (!minSmadret)
              { rammer(tilstand,*t);
                minSmadret=true;
                minSmadretTid=0;
                minHjul1.minXhastighed+=2.0*(minHjul1.minX-minX);
                minHjul1.minYhastighed+=2.0*(minHjul1.minY-minY);
                minHjul1.minZhastighed+=2.0*(minHjul1.minZ-minZ);
                minHjul2.minXhastighed+=2.0*(minHjul2.minX-minX);
                minHjul2.minYhastighed+=2.0*(minHjul2.minY-minY);
                minHjul2.minZhastighed+=2.0*(minHjul2.minZ-minZ);
                minHjul3.minXhastighed+=2.0*(minHjul3.minX-minX);
                minHjul3.minYhastighed+=2.0*(minHjul3.minY-minY);
                minHjul3.minZhastighed+=2.0*(minHjul3.minZ-minZ);
                minHjul4.minXhastighed+=2.0*(minHjul4.minX-minX);
                minHjul4.minYhastighed+=2.0*(minHjul4.minY-minY);
                minHjul4.minZhastighed+=2.0*(minHjul4.minZ-minZ);
                Mix_PlayChannel(2, LydeBibliotek["bang"], 0 );
              }
            }
            float d2=dist(t->minX2,t->minY2,t->minZ2,minX,minY,minZ);
            if (d2<=0.3)
            { // Rammer
              if (!minSmadret)
              { rammer(tilstand,*t);
                minSmadret=true;
                minSmadretTid=0;
                minHjul1.minXhastighed+=2.0*(minHjul1.minX-minX);
                minHjul1.minYhastighed+=2.0*(minHjul1.minY-minY);
                minHjul1.minZhastighed+=2.0*(minHjul1.minZ-minZ);
                minHjul2.minXhastighed+=2.0*(minHjul2.minX-minX);
                minHjul2.minYhastighed+=2.0*(minHjul2.minY-minY);
                minHjul2.minZhastighed+=2.0*(minHjul2.minZ-minZ);
                minHjul3.minXhastighed+=2.0*(minHjul3.minX-minX);
                minHjul3.minYhastighed+=2.0*(minHjul3.minY-minY);
                minHjul3.minZhastighed+=2.0*(minHjul3.minZ-minZ);
                minHjul4.minXhastighed+=2.0*(minHjul4.minX-minX);
                minHjul4.minYhastighed+=2.0*(minHjul4.minY-minY);
                minHjul4.minZhastighed+=2.0*(minHjul4.minZ-minZ);
                Mix_PlayChannel(2, LydeBibliotek["bang"], 0 );
              }
            }
            float d3=dist(t->minX3,t->minY3,t->minZ3,minX,minY,minZ);
            if (d3<=0.3)
            { // Rammer
              if (!minSmadret)
              { rammer(tilstand,*t);
                minSmadret=true;
                minSmadretTid=0;
                minHjul1.minXhastighed+=2.0*(minHjul1.minX-minX);
                minHjul1.minYhastighed+=2.0*(minHjul1.minY-minY);
                minHjul1.minZhastighed+=2.0*(minHjul1.minZ-minZ);
                minHjul2.minXhastighed+=2.0*(minHjul2.minX-minX);
                minHjul2.minYhastighed+=2.0*(minHjul2.minY-minY);
                minHjul2.minZhastighed+=2.0*(minHjul2.minZ-minZ);
                minHjul3.minXhastighed+=2.0*(minHjul3.minX-minX);
                minHjul3.minYhastighed+=2.0*(minHjul3.minY-minY);
                minHjul3.minZhastighed+=2.0*(minHjul3.minZ-minZ);
                minHjul4.minXhastighed+=2.0*(minHjul4.minX-minX);
                minHjul4.minYhastighed+=2.0*(minHjul4.minY-minY);
                minHjul4.minZhastighed+=2.0*(minHjul4.minZ-minZ);
                Mix_PlayChannel(2, LydeBibliotek["bang"], 0 );
              }
            }
          }
        }
      }
    }
  }

  // Flyt hjulene
  minHjul1.Bevaeg(ticks,tilstand);
  minHjul2.Bevaeg(ticks,tilstand);
  minHjul3.Bevaeg(ticks,tilstand);
  minHjul4.Bevaeg(ticks,tilstand);
  // Flyt bilen imellem hjulene
  float minX0=minX;
  float minY0=minY;
  float minZ0=minZ;
  minX=(minHjul1.minX+minHjul2.minX+minHjul3.minX+minHjul4.minX)/4.0;
  minY=(minHjul1.minY+minHjul2.minY+minHjul3.minY+minHjul4.minY)/4.0;
  minZ=(minHjul1.minZ+minHjul2.minZ+minHjul3.minZ+minHjul4.minZ)/4.0;
  minFart=sqrt(sqr(minX-minX0)+sqr(minY-minY0)+sqr(minZ-minZ0))*1000.0/float(ticks);
  // Roter bilen bedst nuligt
  float h14_cos, h14_sin, v14_cos,v14_sin;
  find_rotation(minHjul4.minX,minHjul4.minY,minHjul4.minZ,minHjul1.minX,minHjul1.minY,minHjul1.minZ,h14_cos,h14_sin,v14_cos,v14_sin);
  float h23_cos, h23_sin, v23_cos, v23_sin;
  find_rotation(minHjul3.minX,minHjul3.minY,minHjul3.minZ,minHjul2.minX,minHjul2.minY,minHjul2.minZ,h23_cos,h23_sin,v23_cos,v23_sin);
  float hr12_cos, hr12_sin, t12_cos, t12_sin;
  find_rotation(minHjul2.minX,minHjul2.minY,minHjul2.minZ,minHjul1.minX,minHjul1.minY,minHjul1.minZ,hr12_cos,hr12_sin,t12_cos,t12_sin);
  float hr43_cos, hr43_sin, t43_cos, t43_sin;
  find_rotation(minHjul3.minX,minHjul3.minY,minHjul3.minZ,minHjul4.minX,minHjul4.minY,minHjul4.minZ,hr43_cos,hr43_sin,t43_cos,t43_sin);
  // Gem rotation
  minH_cos=(h14_cos+h23_cos)/2.0;
  minH_sin=(h14_sin+h23_sin)/2.0;
  minH=atan2(minH_sin,minH_cos);
  minH_cos=cos(minH); // Recompute to normalize
  minH_sin=sin(minH); // Recompute to normalize
  minV_cos=(v14_cos+v23_cos)/2.0;
  minV_sin=(v14_sin+v23_sin)/2.0;
  minV=atan2(minV_sin,minV_cos);
  minV_cos=cos(minV); // Recompute to normalize
  minV_sin=sin(minV); // Recompute to normalize
  minT_cos=(t12_cos+t43_cos)/2.0;
  minT_sin=(t12_sin+t43_sin)/2.0;
  minT=atan2(minT_sin,minT_cos);
  minT_cos=cos(minT); // Recompute to normalize
  minT_sin=sin(minT); // Recompute to normalize

  // Sæt hjulene på bilen igen
  if (!minSmadret)
  {
    // Placer hjul1
    float h1x,h1y,h1z;
    roter3d(2.0,1.0,0.0,minH_cos,minH_sin,minV_cos,minV_sin,minT_cos,minT_sin,h1x,h1y,h1z);
    minHjul1.minXhastighed+=(minX+h1x-minHjul1.minX);
    minHjul1.minYhastighed+=(minY+h1y-minHjul1.minY);
    minHjul1.minZhastighed+=(minZ+h1z-minHjul1.minZ);
    minHjul1.minX=minX+h1x;
    minHjul1.minY=minY+h1y;
    minHjul1.minZ=minZ+h1z;
    // Placer hjul 2
    float h2x;
    float h2y;
    float h2z;
    roter3d(2.0,-1.0,0.0,minH_cos,minH_sin,minV_cos,minV_sin,minT_cos,minT_sin,h2x,h2y,h2z);
    minHjul2.minXhastighed+=(minX+h2x-minHjul2.minX);
    minHjul2.minYhastighed+=(minY+h2y-minHjul2.minY);
    minHjul2.minZhastighed+=(minZ+h2z-minHjul2.minZ);
    minHjul2.minX=minX+h2x;
    minHjul2.minY=minY+h2y;
    minHjul2.minZ=minZ+h2z;
    // Placer hjul 3
    float h3x;
    float h3y;
    float h3z;
    roter3d(-2.0,-1.0,0.0,minH_cos,minH_sin,minV_cos,minV_sin,minT_cos,minT_sin,h3x,h3y,h3z);
    minHjul3.minXhastighed+=(minX+h3x-minHjul3.minX);
    minHjul3.minYhastighed+=(minY+h3y-minHjul3.minY);
    minHjul3.minZhastighed+=(minZ+h3z-minHjul3.minZ);
    minHjul3.minX=minX+h3x;
    minHjul3.minY=minY+h3y;
    minHjul3.minZ=minZ+h3z;
    // Placer hjul 4
    float h4x;
    float h4y;
    float h4z;
    roter3d(-2.0,1.0,0.0,minH_cos,minH_sin,minV_cos,minV_sin,minT_cos,minT_sin,h4x,h4y,h4z);
    minHjul4.minXhastighed+=(minX+h4x-minHjul4.minX);
    minHjul4.minYhastighed+=(minY+h4y-minHjul4.minY);
    minHjul4.minZhastighed+=(minZ+h4z-minHjul4.minZ);
    minHjul4.minX=minX+h4x;
    minHjul4.minY=minY+h4y;
    minHjul4.minZ=minZ+h4z;
  }
  // Afspil lyd
  if (!minSmadret)
  { Mix_Volume(5, max(0,50-minAccellererStart/10) );
    Mix_Volume(6, max(0,(50-minSkridStart)*(minFart>=10.0?1:0)) );
  }
  else
  { Mix_Volume(5, 0 );
    Mix_Volume(6, 0 );
  }
  minAccellererStart=min(400,minAccellererStart+(int)ticks);
  minSkridStart=min(400,minSkridStart+(int)ticks);
} // }}}
bool Bil::Forsvind() // {{{
{ return minZ<=-1.0;
} // }}}
string Bil::TilTekst() // {{{
{ stringstream resultat;
  resultat << minHjul1.TilTekst() << " "
           << minHjul2.TilTekst() << " "
           << minHjul3.TilTekst() << " "
           << minHjul4.TilTekst() << " "
           << minSmadret << " "
           << minSmadretTid;
  return resultat.str();
} // }}}
void Bil::Frem(size_t ticks) // {{{
{ if (minHjul1.minKontakt)
  { float hastighed=dist(0.0,0.0,0.0,minHjul1.minXhastighed,minHjul1.minYhastighed,minHjul1.minZhastighed);
    minHjul1.minXhastighed+=(1.0+hastighed*gear_faktor)*minHjul1.minH_cos*minV_cos*frem_faktor*ticks;
    minHjul1.minYhastighed+=(1.0+hastighed*gear_faktor)*minHjul1.minH_sin*minV_cos*frem_faktor*ticks;
    minHjul1.minZhastighed+=(1.0+hastighed*gear_faktor)*minV_sin*frem_faktor*ticks;
    minHjul1.Spin(0.004);
  }
  else
    minHjul1.Spin(0.008);
  if (minHjul2.minKontakt)
  { float hastighed=dist(0.0,0.0,0.0,minHjul2.minXhastighed,minHjul2.minYhastighed,minHjul2.minZhastighed);
    minHjul2.minXhastighed+=(1.0+hastighed*gear_faktor)*minHjul2.minH_cos*minV_cos*frem_faktor*ticks;
    minHjul2.minYhastighed+=(1.0+hastighed*gear_faktor)*minHjul2.minH_sin*minV_cos*frem_faktor*ticks;
    minHjul2.minZhastighed+=(1.0+hastighed*gear_faktor)*minV_sin*frem_faktor*ticks;
    minHjul2.Spin(0.004);
  }
  else
    minHjul2.Spin(0.008);
  if (minHjul3.minKontakt)
  { float hastighed=dist(0.0,0.0,0.0,minHjul3.minXhastighed,minHjul3.minYhastighed,minHjul3.minZhastighed);
    minHjul3.minXhastighed+=(1.0+hastighed*gear_faktor)*minHjul3.minH_cos*minV_cos*frem_faktor*ticks;
    minHjul3.minYhastighed+=(1.0+hastighed*gear_faktor)*minHjul3.minH_sin*minV_cos*frem_faktor*ticks;
    minHjul3.minZhastighed+=(1.0+hastighed*gear_faktor)*minV_sin*frem_faktor*ticks;
    minHjul3.Spin(0.004);
  }
  else
    minHjul3.Spin(0.008);
  if (minHjul4.minKontakt)
  { float hastighed=dist(0.0,0.0,0.0,minHjul4.minXhastighed,minHjul4.minYhastighed,minHjul4.minZhastighed);
    minHjul4.minXhastighed+=(1.0+hastighed*gear_faktor)*minHjul4.minH_cos*minV_cos*frem_faktor*ticks;
    minHjul4.minYhastighed+=(1.0+hastighed*gear_faktor)*minHjul4.minH_sin*minV_cos*frem_faktor*ticks;
    minHjul4.minZhastighed+=(1.0+hastighed*gear_faktor)*minV_sin*frem_faktor*ticks;
    minHjul4.Spin(0.004);
  }
  else
    minHjul4.Spin(0.008);

  // Registrer lyd
  minAccellererStart=0;
} // }}}
void Bil::Tilbage(size_t ticks) // {{{
{ if (minHjul1.minKontakt)
    { minHjul1.minXhastighed-=minHjul1.minH_cos*minV_cos*0.05*ticks;
      minHjul1.minYhastighed-=minHjul1.minH_sin*minV_cos*0.05*ticks;
      minHjul1.minZhastighed-=minV_sin*0.05*ticks;
      minHjul1.Spin(-0.002);
    }
    else
      minHjul1.Spin(-0.004);
    if (minHjul2.minKontakt)
    { minHjul2.minXhastighed-=minHjul2.minH_cos*minV_cos*0.05*ticks;
      minHjul2.minYhastighed-=minHjul2.minH_sin*minV_cos*0.05*ticks;
      minHjul2.minZhastighed-=minV_sin*0.05*ticks;
      minHjul2.Spin(-0.002);
    }
    else
      minHjul2.Spin(-0.004);
    if (minHjul3.minKontakt)
    { minHjul3.minXhastighed-=minHjul3.minH_cos*minV_cos*0.05*ticks;
      minHjul3.minYhastighed-=minHjul3.minH_sin*minV_cos*0.05*ticks;
      minHjul3.minZhastighed-=minV_sin*0.05*ticks;
      minHjul3.Spin(-0.002);
    }
    else
      minHjul3.Spin(-0.004);
    if (minHjul4.minKontakt)
    { minHjul4.minXhastighed-=minHjul4.minH_cos*minV_cos*0.05*ticks;
      minHjul4.minYhastighed-=minHjul4.minH_sin*minV_cos*0.05*ticks;
      minHjul4.minZhastighed-=minV_sin*0.05*ticks;
      minHjul4.Spin(-0.002);
    }
    else
      minHjul4.Spin(-0.004);
} // }}}
void Bil::SetRetning(bool venstre, bool hoejre) // {{{
{ minHjul1.SetRetning(minH+(venstre?0.15:0.0)-(hoejre?0.15:0.0),minT);
  minHjul2.SetRetning(minH+(venstre?0.15:0.0)-(hoejre?0.15:0.0),minT);
  minHjul3.SetRetning(minH,minT);
  minHjul4.SetRetning(minH,minT);
  // Registrer lyd
  if (venstre || hoejre)
    minSkridStart=0;
} // }}}
void Bil::Tegn(SDL_Surface *dest, vector<float>&zbuf, Tilstand &tilstand) // {{{
{ tegn_ting(dest,zbuf,tilstand,*this,ingen_teksturer);
  tegn_ting(dest,zbuf,tilstand,minHjul1,ingen_teksturer);
  tegn_ting(dest,zbuf,tilstand,minHjul2,ingen_teksturer);
  tegn_ting(dest,zbuf,tilstand,minHjul3,ingen_teksturer);
  tegn_ting(dest,zbuf,tilstand,minHjul4,ingen_teksturer);
} // }}}
void Bil::TilCheckpoint(Tilstand &tilstand) // {{{
{ minX=tilstand.checkpoint_x;
  minY=tilstand.checkpoint_y;
  minZ=tilstand.checkpoint_z;
  minH=0;
  minV=0;
  minT=0;
  minH_cos=cos(minH);
  minH_sin=sin(minH);
  minV_cos=cos(minV);
  minV_sin=sin(minV);
  minT_cos=cos(minT);
  minT_sin=sin(minT);
  // Placer hjul1
  float h1x,h1y,h1z;
  roter3d(2.0,1.0,0.0,minH_cos,minH_sin,minV_cos,minV_sin,minT_cos,minT_sin,h1x,h1y,h1z);
  minHjul1.minXhastighed=0;
  minHjul1.minYhastighed=0;
  minHjul1.minZhastighed=0;
  minHjul1.minX=minX+h1x;
  minHjul1.minY=minY+h1y;
  minHjul1.minZ=minZ+h1z;
  // Placer hjul 2
  float h2x;
  float h2y;
  float h2z;
  roter3d(2.0,-1.0,0.0,minH_cos,minH_sin,minV_cos,minV_sin,minT_cos,minT_sin,h2x,h2y,h2z);
  minHjul2.minXhastighed=0;
  minHjul2.minYhastighed=0;
  minHjul2.minZhastighed=0;
  minHjul2.minX=minX+h2x;
  minHjul2.minY=minY+h2y;
  minHjul2.minZ=minZ+h2z;
  // Placer hjul 3
  float h3x;
  float h3y;
  float h3z;
  roter3d(-2.0,-1.0,0.0,minH_cos,minH_sin,minV_cos,minV_sin,minT_cos,minT_sin,h3x,h3y,h3z);
  minHjul3.minXhastighed=0;
  minHjul3.minYhastighed=0;
  minHjul3.minZhastighed=0;
  minHjul3.minX=minX+h3x;
  minHjul3.minY=minY+h3y;
  minHjul3.minZ=minZ+h3z;
  // Placer hjul 4
  float h4x;
  float h4y;
  float h4z;
  roter3d(-2.0,1.0,0.0,minH_cos,minH_sin,minV_cos,minV_sin,minT_cos,minT_sin,h4x,h4y,h4z);
  minHjul4.minXhastighed=0;
  minHjul4.minYhastighed=0;
  minHjul4.minZhastighed=0;
  minHjul4.minX=minX+h4x;
  minHjul4.minY=minY+h4y;
  minHjul4.minZ=minZ+h4z;
} // }}}

// Implementer fly som en ting
Fly::Fly() // {{{
: Avatar("fly",0.0,0.0,0.0,0.0,0.0,0.0,0.0)
//, minPropel(2, 0, 0, 0, 0, 0, 0)
{ minSmadret=false;
  minZhastighed=0.0;
  minKontakt=false;
  // Start motor
  Mix_Volume(5, 0);
  Mix_PlayChannel(5, LydeBibliotek["fly_motor"], -1 );
} // }}}
Fly::Fly(stringstream &ss) // {{{
: Avatar()
//, minPropel(ss)
{ ss>>minSmadret;
  ss>>minSmadretTid;
  ss>>minZhastighed;
  ss>>minKontakt;
  // Start motor
  Mix_Volume(5, 0);
  Mix_PlayChannel(5, LydeBibliotek["fly_motor"], -1 );
} // }}}
Fly::Fly(float x, float y, float z, float h, float v, float t, float fart) // {{{
: Avatar("fly",x,y,z,h,v,t,fart)
//, minPropel(x+2.0,y,z,h,v,t,fart)
{ minSmadret=false;
  minZhastighed=0.0;
  minKontakt=false;
  // Start motor
  Mix_Volume(5, 0);
  Mix_PlayChannel(5, LydeBibliotek["fly_motor"], -1 );
} // }}}
Fly::~Fly() // {{{
{
  // Stop motor
  Mix_HaltChannel(5);
} // }}}
string Fly::Type() // {{{
{ return "fly";
} // }}}
void Fly::Bevaeg(size_t ticks, Tilstand &tilstand) // {{{
{ // Tyngdekraft
  float t=min(0.0,-tyngdekraft+minFart*0.0013);
  minZhastighed+=t;
  minZ+=minZhastighed*ticks*0.001;

  // Drej
  minH-=minT*0.00005*minFart*ticks;
  minH_cos=cos(minH);
  minH_sin=sin(minH);

  // Find punkter på flyet
  float fremX,fremY,fremZ;
  roter3d(1.0,0.0,0.0,minH_cos,minH_sin,minV_cos,minV_sin,minT_cos,minT_sin,fremX,fremY,fremZ);
  float hjulX,hjulY,hjulZ;
  roter3d(0.0,0.0,-0.5,minH_cos,minH_sin,minV_cos,minV_sin,minT_cos,minT_sin,hjulX,hjulY,hjulZ);
  float venstreX,venstreY,venstreZ;
  roter3d(0.0,1.5,0.0,minH_cos,minH_sin,minV_cos,minV_sin,minT_cos,minT_sin,venstreX,venstreY,venstreZ);
  float hoejreX,hoejreY,hoejreZ;
  roter3d(0.0,-1.5,0.0,minH_cos,minH_sin,minV_cos,minV_sin,minT_cos,minT_sin,hoejreX,hoejreY,hoejreZ);
  // Bevæg fremad
  minX+=fremX*minFart*ticks*0.001;
  minY+=fremY*minFart*ticks*0.001;
  minZ+=fremZ*minFart*ticks*0.001;

  minFart*=friktion_fri;
  minZhastighed*=friktion_fri;

  // Håndter styrt
  if (minSmadret)
  { if (minSmadretTid>=5000)
    { minSmadret=false;
      minSmadretTid=0;
      TilCheckpoint(tilstand);
    }
    else
      minSmadretTid+=ticks;
  }
  else
  {
    minKontakt=false;
    // Bliver flyet ramt
    for (int x=Omraader::Afsnit(int(minX)-1); x<=Omraader::Afsnit(int(minX)+1); ++x)
    { for (int y=Omraader::Afsnit(int(minY)-1); y<=Omraader::Afsnit(int(minY)+1); ++y)
      { for (int z=Omraader::Afsnit(int(minZ)-1); z<=Omraader::Afsnit(int(minZ)+1); ++z)
        { vector<Trekant*> &omraade(tilstand.omraader.Omraade(x,y,z));
          for (size_t i=0; i<omraade.size(); ++i)
          { const Trekant *t=omraade[i];
            // Rammer hjul
            float dh1=dist(t->minX1,t->minY1,t->minZ1,minX+hjulX,minY+hjulY,minZ+hjulZ);
            if (dh1<=0.5) // Rammer hjul
            { rammer(tilstand,*t);
              float retningX=minX+hjulX-t->minX1;
              float retningY=minY+hjulY-t->minY1;
              float retningZ=minZ+hjulZ-t->minZ1;
              minX+=(0.5-dh1)*retningX; // Correct position
              minY+=(0.5-dh1)*retningY; // Correct position
              minZ+=(0.5-dh1)*retningZ; // Correct position
              minKontakt=true;
            }
            float dh2=dist(t->minX2,t->minY2,t->minZ2,minX+hjulX,minY+hjulY,minZ+hjulZ);
            if (dh2<=0.5) // Rammer hjul
            { rammer(tilstand,*t);
              float retningX=minX+hjulX-t->minX2;
              float retningY=minY+hjulY-t->minY2;
              float retningZ=minZ+hjulZ-t->minZ2;
              minX+=(0.5-dh2)*retningX; // Correct position
              minY+=(0.5-dh2)*retningY; // Correct position
              minZ+=(0.5-dh2)*retningZ; // Correct position
              minKontakt=true;
            }
            float dh3=dist(t->minX3,t->minY3,t->minZ3,minX+hjulX,minY+hjulY,minZ+hjulZ);
            if (dh3<=0.5) // Rammer hjul
            { rammer(tilstand,*t);
              float retningX=minX+hjulX-t->minX3;
              float retningY=minY+hjulY-t->minY3;
              float retningZ=minZ+hjulZ-t->minZ3;
              minX+=(0.5-dh3)*retningX; // Correct position
              minY+=(0.5-dh3)*retningY; // Correct position
              minZ+=(0.5-dh3)*retningZ; // Correct position
              minKontakt=true;
            }
            // Rammer front
            float df1=dist(t->minX1,t->minY1,t->minZ1,minX+fremX,minY+fremY,minZ+fremZ);
            float df2=dist(t->minX2,t->minY2,t->minZ2,minX+fremX,minY+fremY,minZ+fremZ);
            float df3=dist(t->minX3,t->minY3,t->minZ3,minX+fremX,minY+fremY,minZ+fremZ);
            float dvv1=dist(t->minX1,t->minY1,t->minZ1,minX+venstreX,minY+venstreY,minZ+venstreZ);
            float dvv2=dist(t->minX2,t->minY2,t->minZ2,minX+venstreX,minY+venstreY,minZ+venstreZ);
            float dvv3=dist(t->minX3,t->minY3,t->minZ3,minX+venstreX,minY+venstreY,minZ+venstreZ);
            float dhv1=dist(t->minX1,t->minY1,t->minZ1,minX+hoejreX,minY+hoejreY,minZ+hoejreZ);
            float dhv2=dist(t->minX2,t->minY2,t->minZ2,minX+hoejreX,minY+hoejreY,minZ+hoejreZ);
            float dhv3=dist(t->minX3,t->minY3,t->minZ3,minX+hoejreX,minY+hoejreY,minZ+hoejreZ);
            if (df1<=0.5 || df2<=0.5 || df3<=0.5 || dvv1<=0.5 || dvv2<=0.5 || dvv3<=0.5 || dhv1<=0.5 || dhv2<=0.5 || dhv3<=0.5) // Rammer andet end hjul
            { rammer(tilstand,*t);
              // Rammer
              if (!minSmadret)
              { minSmadret=true;
                minSmadretTid=0;
                Mix_PlayChannel(2, LydeBibliotek["bang"], 0 );
              }
            }
          }
        }
      }
    }
  }
  if (!minSmadret)
  { Mix_Volume(5, int(minFart*3.0) );
  }
  else
  { Mix_Volume(5, 0 );
  }
} // }}}
bool Fly::Forsvind() // {{{
{ return minZ<=-1.0;
} // }}}
string Fly::TilTekst() // {{{
{ stringstream resultat;
  resultat << minSmadret << " "
           << minSmadretTid;
  return resultat.str();
} // }}}
void Fly::Frem(size_t ticks) // {{{
{ minFart+=frem_faktor*ticks*gear_faktor;
} // }}}
void Fly::Tilbage(size_t ticks) // {{{
{ minFart*=0.9;
} // }}}
void Fly::SetRetning(bool venstre, bool hoejre) // {{{
{ //minH+=(venstre?0.001:0.0)-(hoejre?0.001:0.0);
  //minH_cos=cos(minH);
  //minH_sin=sin(minH);
  //if (minFart<10.0 && minV>0.01-(M_PI/2.0))
  //  minV-=0.01;
  //if (minFart>10.0 && minV<(M_PI/2.0)-0.01)
  //  minV+=0.01;
  minV=min(M_PI/2.0,minFart/7.0-M_PI/2.0);
  if (minKontakt && minV<0.0)
    minV=0.0;
  minV_cos=cos(minV);
  minV_sin=sin(minV);
  minT-=(venstre?0.0005:0.0)-(hoejre?0.0005:0.0);
  if (minKontakt)
    minT*=friktion;
  else
    minT*=friktion_fri; //0.9988;
  minT_cos=cos(minT);
  minT_sin=sin(minT);
} // }}}
void Fly::Tegn(SDL_Surface *dest, vector<float>&zbuf, Tilstand &tilstand) // {{{
{ tegn_ting(dest,zbuf,tilstand,*this,ingen_teksturer);
} // }}}
void Fly::TilCheckpoint(Tilstand &tilstand) // {{{
{ minX=tilstand.checkpoint_x;
  minY=tilstand.checkpoint_y;
  minZ=tilstand.checkpoint_z;
  minH=0;
  minV=0;
  minT=0;
  minZhastighed=0.0;
  minFart=5.0;
  minH_cos=cos(minH);
  minH_sin=sin(minH);
  minV_cos=cos(minV);
  minV_sin=sin(minV);
  minT_cos=cos(minT);
  minT_sin=sin(minT);
} // }}}

// Implementer baad som en ting
Baad::Baad() // {{{
: Avatar("baad",0.0,0.0,0.0,0.0,0.0,0.0,0.0)
{ minSmadret=false;
  minXhastighed=0.0;
  minYhastighed=0.0;
  minZhastighed=0.0;
  minKontakt=false;
  // Start motor
  Mix_Volume(5, 0);
  Mix_PlayChannel(5, LydeBibliotek["fly_motor"], -1 );
} // }}}
Baad::Baad(stringstream &ss) // {{{
: Avatar()
//, minPropel(ss)
{ ss>>minSmadret;
  ss>>minSmadretTid;
  ss>>minXhastighed;
  ss>>minYhastighed;
  ss>>minZhastighed;
  ss>>minKontakt;
  // Start motor
  Mix_Volume(5, 0);
  Mix_PlayChannel(5, LydeBibliotek["fly_motor"], -1 );
} // }}}
Baad::Baad(float x, float y, float z, float h, float v, float t, float fart) // {{{
: Avatar("baad",x,y,z,h,v,t,fart)
{ minSmadret=false;
  minXhastighed=0.0;
  minYhastighed=0.0;
  minZhastighed=0.0;
  minKontakt=false;
  // Start motor
  Mix_Volume(5, 0);
  Mix_PlayChannel(5, LydeBibliotek["baad_motor"], -1 );
} // }}}
Baad::~Baad() // {{{
{
  // Stop motor
  Mix_HaltChannel(5);
} // }}}
string Baad::Type() // {{{
{ return "baad";
} // }}}
void Baad::Bevaeg(size_t ticks, Tilstand &tilstand) // {{{
{ minKontakt=false;
  for (int x=Omraader::Afsnit(int(minX)-1); x<=Omraader::Afsnit(int(minX)+1); ++x)
  { for (int y=Omraader::Afsnit(int(minY)-1); y<=Omraader::Afsnit(int(minY)+1); ++y)
    { for (int z=Omraader::Afsnit(int(minZ)-1); z<=Omraader::Afsnit(int(minZ)+1); ++z)
      { vector<Trekant*> &omraade(tilstand.omraader.Omraade(x,y,z));
        for (size_t i=0; i<omraade.size(); ++i)
        { const Trekant *t=omraade[i];
          float d1=dist(t->minX1,t->minY1,t->minZ1,minX,minY,minZ);
          if (d1<=0.5) // Hjul radius
          { // Rammer
            rammer(tilstand,*t);
            minKontakt=true;
            float retningX=minX-t->minX1;
            float retningY=minY-t->minY1;
            float retningZ=minZ-t->minZ1;
            if (d1<=0.001f)
            { retningX=0.1f;
              d1=0.1f;
            }
            minX+=(0.5-d1)*retningX; // Correct position
            minY+=(0.5-d1)*retningY; // Correct position
            minZ+=(0.5-d1)*retningZ; // Correct position
            minXhastighed+=retningX*(abs(minXhastighed*1.0)+1.0)*0.03f; // Bounce
            minYhastighed+=retningY*(abs(minYhastighed*1.0)+1.0)*0.03f; // Bounce
            minZhastighed+=retningZ*(abs(minZhastighed*1.0)+1.0)*0.03f; // Bounce
          }
          float d2=dist(t->minX2,t->minY2,t->minZ2,minX,minY,minZ);
          if (d2<=0.5)
          { // Rammer
            rammer(tilstand,*t);
            minKontakt=true;
            float retningX=minX-t->minX2;
            float retningY=minY-t->minY2;
            float retningZ=minZ-t->minZ2;
            if (d2<=0.001f)
            { retningX=0.1f;
              d2=0.1f;
            }
            minX+=(0.5-d2)*retningX; // Correct position
            minY+=(0.5-d2)*retningY; // Correct position
            minZ+=(0.5-d2)*retningZ; // Correct position
            minXhastighed+=retningX*(abs(minXhastighed*1.0)+1.0)*0.03f; // Bounce
            minYhastighed+=retningY*(abs(minYhastighed*1.0)+1.0)*0.03f; // Bounce
            minZhastighed+=retningZ*(abs(minZhastighed*1.0)+1.0)*0.03f; // Bounce
          }
          float d3=dist(t->minX3,t->minY3,t->minZ3,minX,minY,minZ);
          if (d3<=0.5)
          { // Rammer
            rammer(tilstand,*t);
            minKontakt=true;
            float retningX=minX-t->minX3;
            float retningY=minY-t->minY3;
            float retningZ=minZ-t->minZ3;
            if (d3<=0.001f)
            { retningX=0.1f;
              d3=0.1f;
            }
            minX+=(0.5-d3)*retningX; // Correct position
            minY+=(0.5-d3)*retningY; // Correct position
            minZ+=(0.5-d3)*retningZ; // Correct position
            minXhastighed+=retningX*(abs(minXhastighed*1.0)+1.0)*0.03f; // Bounce
            minYhastighed+=retningY*(abs(minYhastighed*1.0)+1.0)*0.03f; // Bounce
            minZhastighed+=retningZ*(abs(minZhastighed*1.0)+1.0)*0.03f; // Bounce
          }
        }
      }
    }
  }
  // Tyngdekraft
  if (minZ>=0.0)
    minZhastighed-=tyngdekraft*ticks;
  else
    minZhastighed+=tyngdekraft*ticks;
  
  // Friktion
  minXhastighed*=minKontakt?friktion:friktion_fri;
  minYhastighed*=minKontakt?friktion:friktion_fri;
  minZhastighed*=minKontakt?friktion:friktion_fri;

  // Udfør bevaegelse
  minX+=minXhastighed*0.001*ticks;
  minY+=minYhastighed*0.001*ticks;
  minZ+=minZhastighed*0.001*ticks;

  // Motorlyd
  float hastighed=dist(0.0,0.0,0.0,minXhastighed,minYhastighed,minZhastighed);
  if (!minSmadret)
  { Mix_Volume(5, int(hastighed*3.0) );
  }
  else
  { Mix_Volume(5, 0 );
  }
} // }}}
bool Baad::Forsvind() // {{{
{ return false;
} // }}}
string Baad::TilTekst() // {{{
{ stringstream resultat;
  resultat << minSmadret << " "
           << minSmadretTid;
  return resultat.str();
} // }}}
void Baad::Frem(size_t ticks) // {{{
{ //if (minY<=1.0)
  { float hastighed=dist(0.0,0.0,0.0,minXhastighed,minYhastighed,minZhastighed);
    minXhastighed+=(2.0)*minH_cos*minV_cos*frem_faktor*ticks;
    minYhastighed+=(2.0)*minH_sin*minV_cos*frem_faktor*ticks;
    //minZhastighed+=(1.0+hastighed*gear_faktor)*minV_sin*frem_faktor*ticks;
  }
  //else
  //{ float hastighed=dist(0.0,0.0,0.0,minXhastighed,minYhastighed,minZhastighed);
  //  minXhastighed+=(10.0+hastighed*gear_faktor)*minH_cos*minV_cos*frem_faktor*ticks*0.1;
  //  minYhastighed+=(10.0+hastighed*gear_faktor)*minH_sin*minV_cos*frem_faktor*ticks*0.1;
  //  //minZhastighed+=(1.0+hastighed*gear_faktor)*minV_sin*frem_faktor*ticks;
  //}
} // }}}
void Baad::Tilbage(size_t ticks) // {{{
{ if (minY<=0)
  { float hastighed=dist(0.0,0.0,0.0,minXhastighed,minYhastighed,minZhastighed);
    minXhastighed+=(1.0-hastighed*gear_faktor)*minH_cos*minV_cos*frem_faktor*ticks;
    minYhastighed+=(1.0-hastighed*gear_faktor)*minH_sin*minV_cos*frem_faktor*ticks;
    //minZhastighed+=(1.0-hastighed*gear_faktor)*minV_sin*frem_faktor*ticks;
  }
  else
  { float hastighed=dist(0.0,0.0,0.0,minXhastighed,minYhastighed,minZhastighed);
    minXhastighed+=(1.0-hastighed*gear_faktor)*minH_cos*minV_cos*frem_faktor*ticks*0.1;
    minYhastighed+=(1.0-hastighed*gear_faktor)*minH_sin*minV_cos*frem_faktor*ticks*0.1;
    //minZhastighed+=(1.0-hastighed*gear_faktor)*minV_sin*frem_faktor*ticks;
  }
} // }}}
void Baad::SetRetning(bool venstre, bool hoejre) // {{{
{ minH+=(venstre?0.001:0.0)-(hoejre?0.001:0.0);
  minH_cos=cos(minH);
  minH_sin=sin(minH);
  minV=0.0;
  minV_cos=cos(minV);
  minV_sin=sin(minV);
  minT-=(venstre?0.0005:0.0)-(hoejre?0.0005:0.0);
  minT*=friktion;
  minT_cos=cos(minT);
  minT_sin=sin(minT);
} // }}}
void Baad::Tegn(SDL_Surface *dest, vector<float>&zbuf, Tilstand &tilstand) // {{{
{ tegn_ting(dest,zbuf,tilstand,*this,ingen_teksturer);
  // Tegn vand

  tegn_ting(dest,zbuf,tilstand,*this,ingen_teksturer);
  string vandstr("vand");
  Ting vand_0_0(vandstr, minX, minY, 0.0, 0.0, 0.0, 0.0, 0.0);
  tegn_ting(dest,zbuf,tilstand,vand_0_0,kort_teksturer,WORLD_SPACE);
} // }}}
void Baad::TilCheckpoint(Tilstand &tilstand) // {{{
{ minX=tilstand.checkpoint_x;
  minY=tilstand.checkpoint_y;
  minZ=tilstand.checkpoint_z;
  minH=0;
  minV=0;
  minT=0;
  minZhastighed=0.0;
  minFart=5.0;
  minH_cos=cos(minH);
  minH_sin=sin(minH);
  minV_cos=cos(minV);
  minV_sin=sin(minV);
  minT_cos=cos(minT);
  minT_sin=sin(minT);
} // }}}

int max(int x, int y) // {{{
{ if (x<=y)
    return y;
  return x;
} // }}}
int min(int x, int y) // {{{
{ if (x<=y)
    return x;
  return y;
} // }}}
void tegn_verden(Tilstand &tilstand, SDL_Surface *skaerm) // {{{
{ vector<float> zbuf(bredde*hoejde,maks_afstand);
  // Tegn baggrund
    SDL_FillRect(skaerm,NULL,SDL_MapRGB(skaerm->format,0,0,0));
    SDL_Rect pos;
    { float h=atan2(tilstand.kamera_h_sin,tilstand.kamera_h_cos);
      float v=atan2(tilstand.kamera_v_sin,tilstand.kamera_v_cos);
      pos.x=(int)(h/(2*M_PI)*(BilledeBibliotek["baggrund"]->w));
      pos.y=(int)(v/(M_PI)*(BilledeBibliotek["baggrund"]->w));
      pos.w=bredde-pos.x;
      pos.h=hoejde-pos.y;
      SDL_BlitSurface(BilledeBibliotek["baggrund"], NULL, skaerm, &pos );
      pos.x=(int)(h/(2*M_PI)*(BilledeBibliotek["baggrund"]->w));
      pos.y=(int)(v/(M_PI)*(BilledeBibliotek["baggrund"]->w));
      pos.x-=BilledeBibliotek["baggrund"]->w;
      pos.w=BilledeBibliotek["baggrund"]->w;
      SDL_BlitSurface(BilledeBibliotek["baggrund"], NULL, skaerm, &pos );
    }
    // Nulsitl z-buffer
    for (size_t i=0; i<bredde*hoejde; ++i)
      zbuf[i]=maks_afstand;

    // Tegn bane
    for (int x=Omraader::Afsnit(int(tilstand.kamera_x-maks_afstand)); x<=Omraader::Afsnit(int(tilstand.kamera_x+maks_afstand)); ++x)
    { for (int y=Omraader::Afsnit(int(tilstand.kamera_y-maks_afstand)); y<=Omraader::Afsnit(int(tilstand.kamera_y+maks_afstand)); ++y)
      { for (int z=Omraader::Afsnit(int(tilstand.kamera_z-maks_afstand)); z<=Omraader::Afsnit(int(tilstand.kamera_z+maks_afstand)); ++z)
        { vector<Trekant*> &omraade(tilstand.omraader.Omraade(x,y,z));
          for (vector<Trekant*>::const_iterator t=omraade.begin(); t!=omraade.end(); ++t)
            tegn_trekant3d(skaerm,zbuf,tilstand,**t,kort_teksturer);
        }
      }
    }

    //Tegn Avatar
    tilstand.mig->Tegn(skaerm,zbuf,tilstand);

    pthread_mutex_lock(&tilstand.klienter_laas);
    for (size_t t=0; t<tilstand.ting.size(); ++t)
    { tegn_ting(skaerm,zbuf,tilstand,*tilstand.ting[t],ingen_teksturer);
    }
    pthread_mutex_unlock(&tilstand.klienter_laas);

    // Tegn stereogram {{{
    if (tilstand.tegn_stereogram)
    { //for (size_t i=0; i<sbuf.size(); ++i)
      //  sbuf[i]=rand();
      vector<Uint8> sbuf(bredde*hoejde*3,0);
      srand(0);
      for (size_t i=0; i<sbuf.size(); ++i)
        sbuf[i]=rand();
      int mbredde=bredde/10;
      for (int y=0; y<hoejde; ++y)
      { for (int x=0; x<bredde; ++x)
        { if (x<mbredde)
            pixelRGBA(skaerm,x,y,sbuf[3*(y*bredde+x)],sbuf[3*(y*bredde+x)+1],sbuf[3*(y*bredde+x)+2],255);
          else
          { float sumdist=0.0;
	    float numdist=0.0;
            for (int i=max(x-2,0); i<min(x+2,bredde-1); ++i)
	    { numdist+=1.0;
              sumdist+=zbuf[y*bredde+i];
	    }
	    float avgdist=sumdist/numdist;
	    int mlen=mbredde-min(mbredde-10,int(float(mbredde-10)/(avgdist/5.0)));
	    int src_x=int(x)-mlen;
            sbuf[3*(y*bredde+x)]=sbuf[3*(y*bredde+src_x)];
            sbuf[3*(y*bredde+x)+1]=sbuf[3*(y*bredde+src_x)+1];
            sbuf[3*(y*bredde+x)+2]=sbuf[3*(y*bredde+src_x)+2];
            pixelRGBA(skaerm,x,y,sbuf[3*(y*bredde+x)],sbuf[3*(y*bredde+x)+1],sbuf[3*(y*bredde+x)+2],255);
            //pixelRGBA(skaerm,x,y,int(avgdist),int(avgdist),int(avgdist),255);
            //pixelRGBA(skaerm,x,y,mlen,mlen,mlen,255);
          }
        }
      }
    } // }}}
} // }}}


// Bevæg spilleren ud fra tilstanden om
// hvilke taster der er trykket, samt
// spillerens retning
inline void bevaeg(Tilstand &t, size_t ticks=10) // {{{
{ if (!t.mig->minSmadret)
  { t.mig->SetRetning(t.tast_venstre,t.tast_hoejre);
    if (t.tast_frem)
      t.mig->Frem(ticks);
    if (t.tast_bak)
      t.mig->Tilbage(ticks);
  }
  for (size_t i=0; i<ticks; ++i)
  {
    // Flyt mig
    t.mig->Bevaeg(1,t);
    for (size_t i=0; i<t.ting.size(); ++i)
    { t.ting[i]->Bevaeg(1,t);
      if (t.ting[i]->Forsvind())
      { delete t.ting[i];
        t.ting.erase(t.ting.begin()+(i--));
      }
    }
  }
  // Flyt kamera
  float dest_x=t.mig->minX-t.mig->minH_cos*10;
  float dest_y=t.mig->minY-t.mig->minH_sin*10;
  float dest_z=t.mig->minZ+2;
  t.kamera_x=t.kamera_x+(dest_x-t.kamera_x)*0.005*ticks;
  t.kamera_y=t.kamera_y+(dest_y-t.kamera_y)*0.005*ticks;
  t.kamera_z=t.kamera_z+(dest_z-t.kamera_z)*0.005*ticks;
  find_rotation(t.kamera_x,t.kamera_y,t.kamera_z,t.mig->minX,t.mig->minY,t.mig->minZ,t.kamera_h_cos,t.kamera_h_sin,t.kamera_v_cos,t.kamera_v_sin);
} // }}}

// Håndter hændelser som tastetryk og
// musebevægelser, og gem ændringerne i
// tilstanden
inline void haandter_haendelse(const SDL_Event &e, Tilstand &t) // {{{
{ switch (e.type)
  { case SDL_KEYDOWN:
      switch (e.key.keysym.sym)
      { case 'a': // LEFT
        case 276: // LEFT
          //cout << "Key Left" << endl;
          t.tast_venstre=true;
          break;
        case 'd': // RIGHT
        case 275: // RIGHT
          //cout << "Key Right" << endl;
          t.tast_hoejre=true;
          break;
        case 's': // DOWN
        case 274: // DOWN
          //cout << "Key Down" << endl;
          t.tast_bak=true;
          break;
        case 'w': // UP
        case 273: // UP
          //cout << "Key Up" << endl;
          t.tast_frem=true;
          break;
        case 32: // MELLEMRUM
          //cout << "Key Space" << endl;
          t.tast_mellemrum=true;
          break;
        case 27: // ESCAPE
          t.quit=true;
          break;
        case 'x':
          t.tegn_stereogram=!t.tegn_stereogram;
        default:
          //cout << "Key Down: " << event.key.keysym.sym << endl;
          break;
      }
      break;
    case SDL_KEYUP:
      switch (e.key.keysym.sym)
      { case 'a': // LEFT
        case 276: // LEFT
          //cout << "Key Left" << endl;
          t.tast_venstre=false;
          break;
        case 'd': // RIGHT
        case 275: // RIGHT
          //cout << "Key Right" << endl;
          t.tast_hoejre=false;
          break;
        case 's': // DOWN
        case 274: // DOWN
          //cout << "Key Down" << endl;
          t.tast_bak=false;
          break;
        case 'w': // UP
        case 273: // UP
          //cout << "Key Up" << endl;
          t.tast_frem=false;
          break;
        case 32: // MELLEMRUM
          //cout << "Key Space" << endl;
          t.tast_mellemrum=false;
          break;
        case 27: // ESCAPE
          t.quit=true;
          break;
        default:
          //cout << "Key Up: " << event.key.keysym.sym << endl;
          break;
      }
      break;
    case SDL_MOUSEBUTTONDOWN:
      //cout << "Mouse buttton down: " << event.button.x << "," << event.button.y << endl;
      break;
    case SDL_MOUSEBUTTONUP:
      //cout << "Mouse buttton up: " << event.button.x << "," << event.button.y << endl;
      break;
    //case SDL_MOUSEMOTION:
    //  //cout << "Mouse moved to: " << event.button.x << "," << event.button.y << endl;
    //  //cout << "Mouse relative: " << event.motion.xrel << "," << event.motion.yrel << endl;
    //  if (e.button.x!=bredde/2 || e.button.y!=hoejde/2)
    //  { t.mig_h-=(float(e.button.x)-float(bredde/2))*M_PI/3000.0;
    //    while (t.mig_h<0)
    //      t.mig_h+=M_PI*2.0;
    //    while (t.mig_h>M_PI*2.0)
    //      t.mig_h-=M_PI*2.0;
    //    t.mig_v-=(float(e.button.y)-float(hoejde/2))*M_PI/3000.0;
    //    if (t.mig_v>M_PI/2.0)
    //      t.mig_v=M_PI/2.0;
    //    if (t.mig_v<-M_PI/2.0)
    //      t.mig_v=-M_PI/2.0;
    //    t.mig_h_cos=cos(t.mig_h);
    //    t.mig_h_sin=sin(t.mig_h);
    //    t.mig_v_cos=cos(t.mig_v);
    //    t.mig_v_sin=sin(t.mig_v);
    //    SDL_WarpMouse(bredde/2,hoejde/2);
    //  }
    //  break;
      //case SDL_VIDEORESIZE:
      //  memcpy(msgData,(void*)&event,sizeof(event));
      //  mq_send(SendQueue, msgData, sizeof(event), MSGTYPE_RESIZE);
      //  noMove=false;
      //  break;
    case SDL_QUIT:
      //cout << "Window closed!" << endl;
      t.quit=true;
      break;
    default:
      //cerr << "Received unknown event: " << e.type << endl;
      break;
  }
} // }}}
inline void menu_haendelse(const SDL_Event &e, int &pos, int &select, int &back) // {{{
{ switch (e.type)
  { case SDL_KEYDOWN:
      switch (e.key.keysym.sym)
      { case 'a': // LEFT
        case 276: // LEFT
          break;
        case 'd': // RIGHT
        case 275: // RIGHT
          break;
        case 's': // DOWN
        case 274: // DOWN
          ++pos;
          break;
        case 'w': // UP
        case 273: // UP
          --pos;
          break;
        case 32: // MELLEMRUM
          select=1;
          break;
        case 27: // ESCAPE
          back=1;
          break;
        default:
          //cout << "Key Down: " << event.key.keysym.sym << endl;
          break;
      }
      break;
    case SDL_QUIT:
      //cout << "Window closed!" << endl;
      back=1;
      break;
    default:
      //cerr << "Received unknown event: " << e.type << endl;
      break;
  }
} // }}}
inline void menu_edit(const SDL_Event &e, std::string &str, int &done) // {{{
{ switch (e.type)
  { case SDL_KEYDOWN:
      if (e.key.keysym.sym>=32 && e.key.keysym.sym<=125)
        str+=e.key.keysym.sym;
      else switch (e.key.keysym.sym)
      { case 27: // ESCAPE
        case 13: // ENTER
          done=1;
          break;
        case 8: // BACKSPACE
        case 127: // DELETE
          if (str.size()>0)
            str=str.substr(0,str.size()-1);
          break;
        default:
          //cout << "Key Down: " << event.key.keysym.sym << endl;
          break;
      }
      break;
    case SDL_QUIT:
      //cout << "Window closed!" << endl;
      done=1;
      break;
    default:
      //cerr << "Received unknown event: " << e.type << endl;
      break;
  }
} // }}}
string trim(const string &str) // {{{
{ string res=str;
  while (res.size()>0 && std::isspace(res[0]))
    res=res.substr(1);
  while (res.size()>0 && std::isspace(res[res.size()-1]))
    res=res.substr(0,res.size()-1);
  return res;
} // }}}
vector<pair<string,string> > laes_config(const string &path) // {{{
{ vector<pair<string,string> >  res;
  ifstream fin(path);
  string line;
  while(!fin.eof())
  { std::getline(fin,line);
    size_t pos=line.find(':');
    if (pos!=string::npos)
    { string a=trim(line.substr(0,pos));
      string b=trim(line.substr(pos+1));
      if (!a.empty())
        res.push_back(pair<string,string>(a,b));
    }
  }
  fin.close();
  return res;
} // }}}
void set_config(const string &navn, const string &vaerdi) // {{{
{ if (navn=="Bredde")
  { stringstream ss;
    ss << vaerdi;
    ss >> bredde;
  }
  else if (navn=="Højde")
  { stringstream ss;
    ss << vaerdi;
    ss >> hoejde;
  }
  else if (navn=="Fuldskærm")
  { stringstream ss;
    ss << vaerdi;
    ss >> fuldskaerm;
  }
  else if (navn=="Maks Afstand")
  { stringstream ss;
    ss << vaerdi;
    ss >> maks_afstand;
  }
  else if (navn=="Min Afstand")
  { stringstream ss;
    ss << vaerdi;
    ss >> min_afstand;
  }
  else if (navn=="Friktion")
  { stringstream ss;
    ss << vaerdi;
    ss >> friktion;
  }
  else if (navn=="Lunfmodstand")
  { stringstream ss;
    ss << vaerdi;
    ss >> friktion_fri;
  }
  else if (navn=="Tyngdekraft")
  { stringstream ss;
    ss << vaerdi;
    ss >> tyngdekraft;
  }
  else
    cerr << "Ukent confik: " << navn << endl;
} // }}}
void skriv_config(const string &path) // {{{
{ ofstream fout(path);
  fout << "Bredde: " << bredde << endl;
  fout << "Højde: " << hoejde << endl;
  fout << "Fuldskærm: " << fuldskaerm << endl;
  fout << "Maks Afstand: " << maks_afstand << endl;
  fout << "Min Afstand: " << min_afstand << endl;
  fout << "Friktion: " << friktion << endl;
  fout << "Lunfmodstand: " << friktion_fri << endl;
  fout << "Tyngdekraft: " << tyngdekraft;
  fout.close();
} // }}}

void spil_bane(Tilstand &tilstand, SDL_Surface *skaerm) // {{{
{ // Kør spil
  size_t ticks0=SDL_GetTicks();
  size_t ticks=SDL_GetTicks();
  size_t frameTicks=1;
  // Sæt startposition
  cout << "Sætter startposition." << endl;
  tilstand.mig->TilCheckpoint(tilstand);
  tilstand.mig->minSmadret=false;
  tilstand.bane_faerdig=false;
  tilstand.quit=false;

  while (!tilstand.quit)
  { tegn_verden(tilstand,skaerm);
    // Beregn og vis FPS // {{{
    // ticks bruges også til hvor
    // meget bevægelse der skal ske
    // imellem hvert billede
    size_t ticks2=SDL_GetTicks();
    frameTicks=ticks2-ticks;
    ticks=ticks2;
    stringstream ss;
    ss << "FPS: " << 1000.0/(float(frameTicks));
    stringRGBA(skaerm,5,5,ss.str().c_str(),0,0,255,255);
    ss.str("");
    ss << "TID: " << float(ticks2-ticks0)/1000.0;
    stringRGBA(skaerm,5,15,ss.str().c_str(),0,0,255,255);
    ss.str("");
    ss << "FART: " << tilstand.mig->minFart;
    stringRGBA(skaerm,5,25,ss.str().c_str(),0,0,255,255);
    // }}}
    // Opdater skærm
    SDL_Flip(skaerm);

    pthread_mutex_lock(&tilstand.klienter_laas);
    // Bevæg spilleren
    for (size_t i=min(100,frameTicks); i>0;)
    { if (i>1)
      { bevaeg(tilstand,1);
        i-=1;
      }
      else
      { bevaeg(tilstand,i);
        i=0;
      }
    }
    pthread_mutex_unlock(&tilstand.klienter_laas);

    // Håndter handlinger
    SDL_Event event;
    while (SDL_PollEvent(&event))
    { haandter_haendelse(event,tilstand);
    }

    if (tilstand.bane_faerdig)
    { cout << "Færdiggjorde bane i " << float(ticks2-ticks0)/1000.0 << " sekunder." << endl;
      tilstand.quit=true;
      break;
    }
  }
} // }}}
void menu_bane(Tilstand &tilstand, SDL_Surface *skaerm, const std::string &koeretoej) // {{{
{ cout << "Menu_bane: " << koeretoej << endl;
  int pos=0;
  int enter=0;
  int tilbage=0;
  size_t ticks=SDL_GetTicks();
  vector<string> baner;
  // Læs folder for liste af baner
  DIR *dir = opendir((string("./menu/")+koeretoej).c_str());
  struct dirent *entry = readdir(dir);
  while (entry != NULL)
  {
    if (entry->d_type == DT_DIR && entry->d_name[0]!='.')
    { cout << "Bane: " << entry->d_name << endl;
      baner.push_back(entry->d_name);
    }
    entry = readdir(dir);
  }
  closedir(dir);

  // Sorter fundne baner
  std::sort(baner.begin(),baner.end());

  vector<SDL_Surface*> bane_billeder;
  for (int i=0; i<baner.size(); ++i)
  { SDL_Surface *billede=IMG_Load((string("./menu/")+koeretoej+"/"+baner[i]+"/billede.png").c_str());
    bane_billeder.push_back(billede);
  }
  while (pos!=baner.size() || enter==0)
  { size_t ticks2=SDL_GetTicks();
    size_t frameTicks=ticks2-ticks;
    ticks=ticks2;
    if (enter==1)
    { indlaes_bane(string("./menu/")+koeretoej+"/"+baner[pos]+"/model.obj", tilstand);
      spil_bane(tilstand,skaerm);
      enter=0;
    }
    SDL_Event event;
    enter=0;
    tilbage=0;
    while (SDL_PollEvent(&event))
    { menu_haendelse(event,pos,enter,tilbage);
    }
    if (pos<0)
      pos=baner.size();
    if (pos>baner.size())
      pos=0;
    // Tegn baggrund
    float h=atan2(tilstand.kamera_h_sin,tilstand.kamera_h_cos);
    h-=0.0001*frameTicks;
    tilstand.kamera_h_cos=cos(h);
    tilstand.kamera_h_sin=sin(h);
    tegn_verden(tilstand,skaerm);
    //SDL_FillRect(skaerm,NULL,SDL_MapRGB(skaerm->format,0,0,0));
    // Draw menu background
    SDL_Rect box;
    box.x=10;
    box.y=10;
    box.w=100;
    box.h=60+baner.size()*30;
    SDL_FillRect(skaerm,&box,SDL_MapRGB(skaerm->format,100,100,50));
    // Tegn billede af valgte bane
    if (pos<bane_billeder.size())
    { box.x=110;
      box.y=10;
      box.w=bane_billeder[pos]->w;
      box.h=bane_billeder[pos]->h;
      SDL_BlitSurface(bane_billeder[pos], NULL, skaerm, &box );
    }
    // Draw selected item
    box.x=20;
    box.y=20+pos*30;
    box.w=80;
    box.h=20;
    SDL_FillRect(skaerm,&box,SDL_MapRGB(skaerm->format,20,100,20));
    // Draw items texts
    for (int i=0; i<baner.size(); ++i)
    stringRGBA(skaerm,25,25+30*i,baner[i].c_str(),0,0,255,255);
    stringRGBA(skaerm,25,25+30*baner.size(),"Tilbage",0,0,255,255);
    SDL_Flip(skaerm);
  }
} // }}}
void menu_config(Tilstand &tilstand, SDL_Surface *skaerm) // {{{
{ //cout << "menu_config" << endl;
  int pos=0;
  int enter=0;
  int tilbage=0;
  size_t ticks=SDL_GetTicks();
  vector<pair<string,string> > config;
  config.push_back(pair<string,string>("Bredde",std::to_string(bredde)));
  config.push_back(pair<string,string>("Højde",std::to_string(hoejde)));
  config.push_back(pair<string,string>("Fuldskærm",std::to_string(fuldskaerm)));
  config.push_back(pair<string,string>("Maks Afstand",std::to_string(maks_afstand)));
  config.push_back(pair<string,string>("Min Afstand",std::to_string(min_afstand)));
  config.push_back(pair<string,string>("Friktion",std::to_string(friktion)));
  config.push_back(pair<string,string>("Luftmodstand",std::to_string(friktion_fri)));
  config.push_back(pair<string,string>("Tyngdekraft",std::to_string(tyngdekraft)));
  int mode=0; // Select config
              // 1=Edit config

  while (pos!=config.size() || mode!=0 || enter==0 )
  { size_t ticks2=SDL_GetTicks();
    size_t frameTicks=ticks2-ticks;
    ticks=ticks2;
    SDL_Event event;
    enter=0;
    tilbage=0;
    while (SDL_PollEvent(&event))
    { if (mode==0)
        menu_haendelse(event,pos,enter,tilbage);
      else
        menu_edit(event,config[pos].second,enter);

      if (pos<0)
        pos=config.size();
      if (pos>config.size())
        pos=0;

      if (pos<config.size() && enter==1)
      { if (mode==1)
          set_config(config[pos].first,config[pos].second);
        // Toggle rediger indstilling
        mode=1-mode;
        enter=0;
      }
    }
    // Tegn baggrund
    float h=atan2(tilstand.kamera_h_sin,tilstand.kamera_h_cos);
    h-=0.0001*frameTicks;
    tilstand.kamera_h_cos=cos(h);
    tilstand.kamera_h_sin=sin(h);
    tegn_verden(tilstand,skaerm);
    //SDL_FillRect(skaerm,NULL,SDL_MapRGB(skaerm->format,0,0,0));
    // Draw menu background
    SDL_Rect box;
    box.x=10;
    box.y=10;
    box.w=300;
    box.h=40+config.size()*30;
    SDL_FillRect(skaerm,&box,SDL_MapRGB(skaerm->format,100,100,50));
    // Draw selected item
    box.x=20;
    box.y=20+pos*30;
    box.w=280;
    box.h=20;
    SDL_FillRect(skaerm,&box,SDL_MapRGB(skaerm->format,20,100,20));
    // Draw items texts
    for (int i=0; i<config.size(); ++i)
    stringRGBA(skaerm,25,25+30*i,(config[i].first+" = "+config[i].second).c_str(),0,0,255,255);
    stringRGBA(skaerm,25,25+30*config.size(),"Tilbage",0,0,255,255);
    SDL_Flip(skaerm);
  }
  skriv_config("./menu/config/config.cfg");
} // }}}
void menu(Tilstand &tilstand, SDL_Surface *skaerm) // {{{
{ int pos=0;
  int enter=0;
  int tilbage=0;
  size_t ticks=SDL_GetTicks();
  while (pos!=4 || enter==0)
  { size_t ticks2=SDL_GetTicks();
    size_t frameTicks=ticks2-ticks;
    ticks=ticks2;
    if (pos==0 && enter==1) // Bil
    { delete tilstand.mig;
      tilstand.mig=new Bil(0.0, 0.0, 10.0, 0.0, 0.0, 0.0, 10.0);
      menu_bane(tilstand,skaerm,"bil");
    }
    else if (pos==1 && enter==1)
    { delete tilstand.mig;
      tilstand.mig=new Fly(0.0, 0.0, 10.0, 0.0, 0.0, 0.0, 10.0);
      menu_bane(tilstand,skaerm,"fly");
    }
    else if (pos==2 && enter==1)
    { delete tilstand.mig;
      tilstand.mig=new Baad(0.0, 0.0, 10.0, 0.0, 0.0, 0.0, 10.0);
      menu_bane(tilstand,skaerm,"baad");
    }
    else if (pos==3 && enter==1)
    { menu_config(tilstand,skaerm);
    }
    SDL_Event event;
    enter=0;
    tilbage=0;
    while (SDL_PollEvent(&event))
    { menu_haendelse(event,pos,enter,tilbage);
    }
    if (pos<0)
      pos=4;
    if (pos>4)
      pos=0;
    // Tegn baggrund
    float h=atan2(tilstand.kamera_h_sin,tilstand.kamera_h_cos);
    h+=0.0001*frameTicks;
    tilstand.kamera_h_cos=cos(h);
    tilstand.kamera_h_sin=sin(h);
    tegn_verden(tilstand,skaerm);
    //SDL_FillRect(skaerm,NULL,SDL_MapRGB(skaerm->format,0,0,0));
    // Draw menu background
    SDL_Rect box;
    box.x=10;
    box.y=10;
    box.w=140;
    box.h=160;
    SDL_FillRect(skaerm,&box,SDL_MapRGB(skaerm->format,100,100,50));
    // Draw selected item
    box.x=20;
    box.y=20+pos*30;
    box.w=120;
    box.h=20;
    SDL_FillRect(skaerm,&box,SDL_MapRGB(skaerm->format,20,100,20));
    // Draw items texts
    stringRGBA(skaerm,25,25,"Bil",0,0,255,255);
    stringRGBA(skaerm,25,55,"Fly",0,0,255,255);
    stringRGBA(skaerm,25,85,"Baad",0,0,255,255);
    stringRGBA(skaerm,25,115,"Indstillinger",0,0,255,255);
    stringRGBA(skaerm,25,145,"Afslut",0,0,255,255);
    SDL_Flip(skaerm);
  }
} // }}}

void *spil(void *t) // {{{
{ Tilstand &tilstand(*(Tilstand*)t);
  SDL_Surface *primary = SDL_SetVideoMode(bredde,hoejde,WINDOWDEAPTH,SDL_HWSURFACE | SDL_DOUBLEBUF | (fuldskaerm?SDL_FULLSCREEN:0));
  SDL_WM_SetCaption("Back On Track","Back On Track");
  SDL_ShowCursor(false);

  // Menu
  menu(tilstand,primary);

  SDL_FreeSurface(primary);
  return NULL;
} // }}}

// main er selve programmet
int main(int argc, char **argv) // {{{{
{ // Opret vindue
  // Initialize SDL {{{
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  int imgFlags = IMG_INIT_PNG;
  if( !( IMG_Init( imgFlags ) & imgFlags ) )
  {
      printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
      return -1;
  }
  //Initialize SDL_mixer
  if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) < 0 )
  {
    printf( "SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError() );
    return -1;
  }
  if(SDLNet_Init()==-1)
  {
    printf("SDLNet_Init: %s\n",SDLNet_GetError());
    SDL_Quit();
    return -1;
  }
  // }}}

  // Opret spillets tilstand
  Tilstand tilstand;
  vector<pthread_t> traade;

  // Indlæs config
  vector<pair<string,string> > cfg=laes_config("./menu/config/config.cfg");
  for (size_t i=0; i<cfg.size(); ++i)
    set_config(cfg[i].first,cfg[i].second);

  // Init biblioteker
  FigurBibliotek["bil"]=smaa_trekanter(skaler_ting(2.3,-1.2,-0.5,-2.3,1.2,0.7,laes_obj("ting/bil1.obj")));
  FigurBibliotek["baad"]=smaa_trekanter(skaler_ting(-2.3,-1.0,-0.1,2.3,1.0,1.7,laes_obj("ting/baad.obj")));
  FigurBibliotek["fly"]=smaa_trekanter(skaler_ting(-1.0,-3.0,-1.0,1.0,3.0,1.0,laes_obj("ting/fly.obj")));
  FigurBibliotek["pengvin"]=smaa_trekanter(skaler_ting(-1,-1,-2,1,1,2,laes_obj("ting/pengvin.obj")));
  FigurBibliotek["hjul"]=smaa_trekanter(skaler_ting(-0.5,-0.2,-0.5,0.5,0.2,0.5,laes_obj("ting/hjul3.obj")));
  vector<Trekant> vandfigur;
  vandfigur.push_back(Trekant(-maks_afstand, -maks_afstand, 0.0, -maks_afstand,  maks_afstand, 0.0, maks_afstand, maks_afstand, 0.0, 11, 11, 111));
  vandfigur.push_back(Trekant(-maks_afstand, -maks_afstand, 0.0,  maks_afstand, -maks_afstand, 0.0, maks_afstand, maks_afstand, 0.0, 11, 11, 111));
  FigurBibliotek["vand"]=smaa_trekanter(vandfigur);

  BilledeBibliotek["sigtekorn"]=IMG_Load("billeder/sigtekorn.png");
  BilledeBibliotek["granat"]=IMG_Load("billeder/granat.png");
  BilledeBibliotek["pistol"]=IMG_Load("billeder/pistol.png");
  BilledeBibliotek["baggrund"]=IMG_Load("billeder/baggrund.jpg");
  BilledeBibliotek["tekstur_vand"]=IMG_Load("billeder/vand.png");
  BilledeBibliotek["tekstur_græs"]=IMG_Load("billeder/græs.jpg");
  BilledeBibliotek["tekstur_asphalt"]=IMG_Load("billeder/asphalt.png");
  BilledeBibliotek["tekstur_mål"]=IMG_Load("billeder/mål.png");
  kort_teksturer[43*256*256+46*256+49]="tekstur_asphalt";
  kort_teksturer[18*256*256+105*256+54]="tekstur_græs";
  kort_teksturer[70*256*256+183*256+73]="tekstur_græs";
  kort_teksturer[0*256*256+159*256+215]="tekstur_mål";
  kort_teksturer[11*256*256+11*256+111]="tekstur_vand";
  LydeBibliotek["hop"]=Mix_LoadWAV("lyde/klang.wav");
  LydeBibliotek["bang"]=Mix_LoadWAV("lyde/bang.wav");
  LydeBibliotek["bil_motor"]=Mix_LoadWAV("lyde/motor.wav");
  LydeBibliotek["bil_skrid"]=Mix_LoadWAV("lyde/bil_skrid.wav");
  LydeBibliotek["fly_motor"]=Mix_LoadWAV("lyde/fly_motor.wav");
  LydeBibliotek["baad_motor"]=Mix_LoadWAV("lyde/baad_motor.wav");


  tilstand.banesti="baner/bane1.obj"; //string(argv[1]);
  indlaes_bane( tilstand.banesti, tilstand);

  // Start spil
  pthread_t spil_traad;
  pthread_create(&spil_traad, NULL, spil, (void*)&tilstand);
  traade.push_back(spil_traad);

  // Håndter afslutning
  void *res;
  while (!traade.empty())
  { pthread_join(traade.back(), &res);
    traade.pop_back();
  }

  Mix_Quit();
  IMG_Quit();
  SDL_Quit();
  return 0;
} // }}}

