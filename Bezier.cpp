//
//  Bezier.cpp
//  OpenGL
// 

#include "Bezier.h"

// **********************************************************************
Bezier::Bezier()
{
    for (int i=0;i<3;i++)
        Coords[i] = Ponto(0,0,0);
    ComprimentoTotalDaCurva = 0;
    cor = rand() % 100;
    //cout << "Cor: " << cor << endl;
}
// **********************************************************************
//
// **********************************************************************
void Bezier::calculaComprimentoDaCurva()
{
    double DeltaT = 1.0/50;
    double t=DeltaT;
    Ponto P1, P2;
    
    ComprimentoTotalDaCurva = 0;
    
    P1 = Calcula(0.0);
    while(t<1.0)
    {
        P2 = Calcula(t);
        ComprimentoTotalDaCurva += calculaDistancia(P1,P2);
        P1 = P2;
        t += DeltaT;
    }
    P2 = Calcula(1.0); // faz o fechamento da curva
    ComprimentoTotalDaCurva += calculaDistancia(P1,P2);
    //cout << "ComprimentoTotalDaCurva: " << ComprimentoTotalDaCurva << endl;
    
}
// **********************************************************************
Bezier::Bezier(Ponto P0, Ponto P1, Ponto P2)
{
    Coords[0] = P0;
    Coords[1] = P1;
    Coords[2] = P2;
    calculaComprimentoDaCurva();
    cor = rand() % 100;
}
// **********************************************************************
Bezier::Bezier(Ponto V[])
{
    for (int i=0;i<3;i++)
        Coords[i] = V[i];
    calculaComprimentoDaCurva();
    cor = rand() % 100;
}
// **********************************************************************
//
// **********************************************************************
Ponto Bezier::Calcula(double t)
{
    Ponto P;
    double UmMenosT = 1-t;
    
    P =  Coords[0] * UmMenosT * UmMenosT + Coords[1] * 2 * UmMenosT * t + Coords[2] * t*t;
    return P;
}
// **********************************************************************
//
// **********************************************************************
double Bezier::CalculaT(double distanciaPercorrida)
{
    return (distanciaPercorrida/ComprimentoTotalDaCurva);
}
// **********************************************************************
//
// **********************************************************************
Ponto Bezier::getPC(int i)
{
    return Coords[i];
}
// **********************************************************************
void Bezier::Traca()
{
    double t=0.0;
    double DeltaT = 1.0/50;
    Ponto P;
    //cout << "DeltaT: " << DeltaT << endl;
    glBegin(GL_LINE_STRIP);
    
    while(t<1.0)
    {
        P = Calcula(t);
        //P.imprime("P: ");
        glVertex2f(P.x, P.y);
        t += DeltaT;
    }
    P = Calcula(1.0); // faz o fechamento da curva
    glVertex2f(P.x, P.y);
    glEnd();
}
// **********************************************************************
//
// **********************************************************************
void Bezier::TracaPoligonoDeControle()
{
    glBegin(GL_LINE_LOOP);
    for(int i=0;i<3;i++)
        glVertex3f(Coords[i].x,Coords[i].y,Coords[i].z);
    glEnd();
    
}

// **********************************************************************
void Bezier::setPC(int i, Ponto P)
{
    if (i >= 0 && i < 3)
        Coords[i] = P;
}

// **********************************************************************
// Cria uma nova curva mantendo continuidade de posição com a curva anterior
// **********************************************************************
Bezier Bezier::CriaCurvaComContinuidadePosicao(const Bezier& curvaAnterior, Ponto P1, Ponto P2)
{
    // O primeiro ponto da nova curva é o último ponto da curva anterior
    Ponto P0 = curvaAnterior.Coords[2];
    return Bezier(P0, P1, P2);
}

// **********************************************************************
// Cria uma nova curva mantendo continuidade de derivada com a curva anterior
// **********************************************************************
Bezier Bezier::CriaCurvaComContinuidadeDerivada(const Bezier& curvaAnterior, Ponto P2)
{
    // O primeiro ponto da nova curva é o último ponto da curva anterior
    Ponto P0 = curvaAnterior.Coords[2];
    
    // Para manter a continuidade da derivada, o primeiro vetor de controle
    // da nova curva deve seguir a mesma direção do último vetor de controle
    // da curva anterior
    Ponto direcao = curvaAnterior.Coords[2] - curvaAnterior.Coords[1];
    Ponto P1 = P0 + direcao;
    
    return Bezier(P0, P1, P2);
}
