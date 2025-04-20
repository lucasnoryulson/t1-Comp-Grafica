    // **********************************************************************
// PUCRS/Escola Politécnica
// COMPUTAÇO GRÁFICA
//
// Programa basico para criar aplicacoes 2D em OpenGL
//
// Marcio Sarroglia Pinho
// pinho@pucrs.br
// **********************************************************************
  
// Para uso no Xcode:
// Abra o menu Product -> Scheme -> Edit Scheme -> Use custom working directory
// Selecione a pasta onde voce descompactou o ZIP que continha este arquivo.
//
//  Para rodar em um terminal Windows, digite
//           mingw32-make -f Makefile.mk 
//

#include <iostream>
#include <cmath>
#include <ctime>
#include <fstream>
#include <vector>


using namespace std;

#ifdef WIN32
#include <windows.h>
#include <glut.h>
#else
#include <sys/time.h>
#endif

#ifdef __APPLE__
#include <GLUT/glut.h>
#endif

#ifdef __linux__
#include <GL/glut.h>
#endif

#include "Ponto.h"
#include "Poligono.h"
#include "Bezier.h"

#include "Temporizador.h"
#include "ListaDeCoresRGB.h"
#include <tuple>

// Definindo os modos de operação
enum ModoOperacao {
    MODO_SEM_CONTINUIDADE = 0,
    MODO_CONTINUIDADE_POSICAO = 1,
    MODO_CONTINUIDADE_DERIVADA = 2
};

// Variável para armazenar o modo atual
ModoOperacao modoAtual = MODO_SEM_CONTINUIDADE;

// Nomes dos modos para exibição
string NomesModos[] = {
    "Modo Sem Continuidade",
    "Modo Continuidade de Posição",
    "Modo Continuidade de Derivada"
};

std::tuple<float, float> rastroMouse;

bool aguardandoCliqueFinalP2 = false;

bool mouseSegurando = false;
Temporizador T;
double AccumDeltaT=0;
Temporizador T2;

Bezier Curvas[20];
unsigned int nCurvas;
Ponto PontosClicados[3];
int nPontoAtual=0;

Poligono PoligonoDeControle;

// Limites lgicos da área de desenho
Ponto Min, Max;

bool desenha = false;

Poligono Mapa, MeiaSeta, Mastro;

float angulo=0.0;

Ponto PosAtualDoMouse;
bool BotaoDown = false;

// Adicionando variável para rastrear a posição atual do mouse
Ponto PosicaoAtualMouse;

double nFrames=0;
double TempoTotal=0;

// Variáveis para controle dos modos
bool primeiraCurva = true;
int pontosNecessarios = 3; // Número de pontos necessários para criar a curva no modo atual

// **********************************************************************
// Imprime o texto S na posicao (x,y), com a cor 'cor'
// **********************************************************************
void printString(string S, int x, int y, int cor)
{
    defineCor(cor);
    glRasterPos3f(x, y, 0); //define posicao na tela
    for (int i = 0; i < S.length(); i++)
    {
        // GLUT_BITMAP_HELVETICA_10
        // GLUT_BITMAP_TIMES_ROMAN_24
        // GLUT_BITMAP_HELVETICA_18
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, S[i]);
    }
}
#include <sstream>
using std::stringstream;


// **********************************************************************
// Converte um nro em string
// **********************************************************************
string toString(float f)
{
    stringstream S;
    S << f;
    return S.str();
}
// **********************************************************************
// Imprime as coordenadas do ponto P na posicao (x,y), com a cor 'cor'
// **********************************************************************
void ImprimePonto(Ponto P, int x, int y, int cor)
{
    string S;
    S = "( " + toString(P.x) + ", " + toString(P.y) + ")" ;
    printString(S, x, y, cor);
}

// **********************************************************************
//  Vetor de mensagens
// **********************************************************************
string Mensagens[] = {
    "Clique o primeiro ponto.",
    "Clique o segundo ponto.",
    "Clique o terceiro ponto."
};
// **********************************************************************
//  Imprime as mensagens do programa.
//  Funcao chamada na 'display'
// **********************************************************************
void ImprimeMensagens()
{
    // Área de status - Mostra o modo atual
    printString("Modo Atual: " + NomesModos[modoAtual], -14, 13, Yellow);
    
    // Instruções para o usuário
    if (nPontoAtual < 3)
        printString(Mensagens[nPontoAtual], -14, 11, Yellow);
    else
        printString("Clique para reiniciar.", -14, 11, Yellow);

    if (nPontoAtual > 0)
    {
        printString("Ultimo ponto clicado: ", -14, 9, Red);
        ImprimePonto(PontosClicados[nPontoAtual - 1], -3, 9, Red);
    }
    
    // Instruções adicionais
    printString("Teclas: 1-Sem Continuidade, 2-Continuidade Posição, 3-Continuidade Derivada", -14, -13, White);
}


// **********************************************************************
//
// **********************************************************************
void animate()
{
    double dt;
    dt = T.getDeltaT();
    AccumDeltaT += dt;
    TempoTotal += dt;
    nFrames++;

    if (AccumDeltaT > 1.0/30) // fixa a atualização da tela em 30
    {
        AccumDeltaT = 0;
        angulo+=2;
        glutPostRedisplay();
    }
    /*
    if (TempoTotal > 5.0)
    {
        cout << "Tempo Acumulado: "  << TempoTotal << " segundos. " ;
        cout << "Nros de Frames sem desenho: " << nFrames << endl;
        cout << "FPS(sem desenho): " << nFrames/TempoTotal << endl;
        TempoTotal = 0;
        nFrames = 0;
    }*/
}
// **********************************************************************
//  void reshape( int w, int h )
//  trata o redimensionamento da janela OpenGL
// **********************************************************************
void reshape( int w, int h )
{
    // Reset the coordinate system before modifying
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Define a area a ser ocupada pela area OpenGL dentro da Janela
    glViewport(0, 0, w, h);
    // Define os limites logicos da area OpenGL dentro da Janela
    glOrtho(Min.x,Max.x, Min.y,Max.y, -10,+10);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}
// **********************************************************************
// **********************************************************************
void DesenhaEixos()
{
    Ponto Meio;
    Meio.x = (Max.x+Min.x)/2;
    Meio.y = (Max.y+Min.y)/2;
    Meio.z = (Max.z+Min.z)/2;

    glBegin(GL_LINES);
    //  eixo horizontal
        glVertex2f(Min.x,Meio.y);
        glVertex2f(Max.x,Meio.y);
    //  eixo vertical
        glVertex2f(Meio.x,Min.y);
        glVertex2f(Meio.x,Max.y);
    glEnd();
}

// **********************************************************************
//
// **********************************************************************
void CarregaModelos()
{
    Mapa.LePoligono("EstadoRS.txt");
    MeiaSeta.LePoligono("MeiaSeta.txt");
    Mastro.LePoligono("Mastro.txt");
}
// **********************************************************************
//
// **********************************************************************
void CriaCurvaNoModoAtual()
{
    switch(modoAtual)
    {
        case MODO_SEM_CONTINUIDADE:
            Curvas[nCurvas] = Bezier(PontosClicados[0], PontosClicados[1], PontosClicados[2]);
            break;
            
        case MODO_CONTINUIDADE_POSICAO:
            if (primeiraCurva) {
                Curvas[nCurvas] = Bezier(PontosClicados[0], PontosClicados[1], PontosClicados[2]);
                primeiraCurva = false;
            } else {
                Curvas[nCurvas] = Bezier::CriaCurvaComContinuidadePosicao(Curvas[nCurvas-1], 
                                                                         PontosClicados[1], 
                                                                         PontosClicados[2]);
            }
            break;
            
        case MODO_CONTINUIDADE_DERIVADA:
            if (primeiraCurva) {
                Curvas[nCurvas] = Bezier(PontosClicados[0], PontosClicados[1], PontosClicados[2]);
                primeiraCurva = false;
            } else {
                Curvas[nCurvas] = Bezier::CriaCurvaComContinuidadeDerivada(Curvas[nCurvas-1], 
                                                                          PontosClicados[2]);
            }
            break;
    }
    nCurvas++;
}
// **********************************************************************
//
// **********************************************************************
void init()
{
    // Define a cor do fundo da tela (AZUL)
    glClearColor(0.0f, 0.0f, 0.f, 1.0f);

    CarregaModelos();
    
    //CriaCurvas();
    
    float d = 15;
    Min = Ponto(-d,-d);
    Max = Ponto(d,d);
}
// **********************************************************************
//
// **********************************************************************
void DesenhaLinha(Ponto P1, Ponto P2)
{
    glBegin(GL_LINES);
        glVertex2f(P1.x,P1.y);
        glVertex2f(P2.x,P2.y);
    glEnd();
}

// **********************************************************************
// **********************************************************************
void DesenhaCurvas()
{
    for(int i=0; i<nCurvas;i++)
    {
        defineCor(Yellow);
        Curvas[i].Traca();
        defineCor(Brown);
        Curvas[i].TracaPoligonoDeControle();
    }
}
// **********************************************************************
//
// **********************************************************************
void DesenhaPontos()
{
    defineCor(IndianRed);
    glPointSize(4);
    glBegin(GL_POINTS);
    for(int i=0; i<nPontoAtual;i++)
    {
        glVertex2f(PontosClicados[i].x, PontosClicados[i].y);
        
    }
    glEnd();
    glPointSize(1);

}
// **********************************************************************
//
// **********************************************************************
void DesenhaMenu()
{
    glPushMatrix();
        glTranslated(11,13,0); // veja o arquivo MeiaSeta.txt
        MeiaSeta.desenhaPoligono();
    glPopMatrix();
}
// **********************************************************************
// Desenha a grade de fundo
// **********************************************************************
void DesenhaGrade()
{
    glColor3f(0.3, 0.3, 0.3);
    glLineWidth(1);
    
    // Linhas verticais
    for(float x = Min.x; x <= Max.x; x += 1.0) {
        glBegin(GL_LINES);
        glVertex2f(x, Min.y);
        glVertex2f(x, Max.y);
        glEnd();
    }
    
    // Linhas horizontais
    for(float y = Min.y; y <= Max.y; y += 1.0) {
        glBegin(GL_LINES);
        glVertex2f(Min.x, y);
        glVertex2f(Max.x, y);
        glEnd();
    }
}

// **********************************************************************
// Desenha a área de ícones
// **********************************************************************
void DesenhaAreaIcones()
{
    // Desenha o painel lateral direito
    glPushMatrix();
    
    // Fundo do painel
    glColor3f(0.2, 0.2, 0.2);
    glBegin(GL_QUADS);
    glVertex2f(10, -15);  // Ajustado para cobrir toda a altura
    glVertex2f(15, -15);
    glVertex2f(15, 15);
    glVertex2f(10, 15);
    glEnd();
    
    // Título da área de ícones
    glColor3f(1.0, 1.0, 1.0);
    printString("MODOS", 10.5, 13, White);
    
    // Desenha os botões dos modos
    float posY = 11;
    float altura = 1.5;
    float espacamento = 2.0;
    
    // Modo sem continuidade
    if (modoAtual == MODO_SEM_CONTINUIDADE)
        glColor3f(0.8, 0.2, 0.2); // Vermelho mais claro quando selecionado
    else
        glColor3f(0.4, 0.4, 0.4);
    
    glBegin(GL_QUADS);
    glVertex2f(10.5, posY);
    glVertex2f(14.5, posY);
    glVertex2f(14.5, posY - altura);
    glVertex2f(10.5, posY - altura);
    glEnd();
    printString("1", 11, posY - 1.0, White);
    printString("Sem Cont.", 11.5, posY - 1.0, White);
    
    // Modo continuidade de posição
    posY -= espacamento;
    if (modoAtual == MODO_CONTINUIDADE_POSICAO)
        glColor3f(0.8, 0.2, 0.2);
    else
        glColor3f(0.4, 0.4, 0.4);
    
    glBegin(GL_QUADS);
    glVertex2f(10.5, posY);
    glVertex2f(14.5, posY);
    glVertex2f(14.5, posY - altura);
    glVertex2f(10.5, posY - altura);
    glEnd();
    printString("2", 11, posY - 1.0, White);
    printString("Cont. Pos.", 11.5, posY - 1.0, White);
    
    // Modo continuidade de derivada
    posY -= espacamento;
    if (modoAtual == MODO_CONTINUIDADE_DERIVADA)
        glColor3f(0.8, 0.2, 0.2);
    else
        glColor3f(0.4, 0.4, 0.4);
    
    glBegin(GL_QUADS);
    glVertex2f(10.5, posY);
    glVertex2f(14.5, posY);
    glVertex2f(14.5, posY - altura);
    glVertex2f(10.5, posY - altura);
    glEnd();
    printString("3", 11, posY - 1.0, White);
    printString("Cont. Der.", 11.5, posY - 1.0, White);
    
    glPopMatrix();
}

// **********************************************************************
// Função para reiniciar o estado quando trocar de modo
// **********************************************************************
void ReiniciaEstado()
{
    nPontoAtual = 0;
    primeiraCurva = true;
    // Limpa as curvas existentes
    nCurvas = 0;
    // Reseta os pontos clicados
    for(int i = 0; i < 3; i++) {
        PontosClicados[i] = Ponto(0,0,0);
    }
}

// **********************************************************************
// Desenha a área de status
// **********************************************************************
void DesenhaAreaStatus()
{
    // Desenha o painel inferior
    glColor3f(0.2, 0.2, 0.2);
    glBegin(GL_QUADS);
    glVertex2f(-15, -15);
    glVertex2f(10, -15);
    glVertex2f(10, -12);
    glVertex2f(-15, -12);
    glEnd();
    
    // Informações de status
    glColor3f(1.0, 1.0, 0.0);
    string status = "Modo Atual: " + NomesModos[modoAtual];
    printString(status, -14, -13.5, Yellow);
    
    // Mostra instruções baseadas no modo atual
    string instrucao;
    if (primeiraCurva) {
        if (nPontoAtual < 3)
            instrucao = Mensagens[nPontoAtual];
        else
            instrucao = "Clique para reiniciar.";
    } else {
        if (modoAtual == MODO_CONTINUIDADE_POSICAO)
            instrucao = "Clique mais " + to_string(3 - nPontoAtual) + " ponto(s) para continuar a curva";
        else if (modoAtual == MODO_CONTINUIDADE_DERIVADA)
            instrucao = "Clique mais " + to_string(2 - nPontoAtual) + " ponto(s) para continuar a curva";
    }
    printString(instrucao, -14, -14.5, White);
}

// **********************************************************************
//  void display( void )
// **********************************************************************
void display( void )
{
    // Limpa a tela com cor de fundo
    glClear(GL_COLOR_BUFFER_BIT);

    // Define os limites lógicos da área OpenGL dentro da Janela
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Desenha a área de desenho (fundo mais escuro)
    glColor3f(0.1, 0.1, 0.1);
    glBegin(GL_QUADS);
    glVertex2f(-15, -12);
    glVertex2f(10, -12);
    glVertex2f(10, 15);
    glVertex2f(-15, 15);
    glEnd();

    // Desenha a grade de fundo
    DesenhaGrade();

    // Desenha os eixos
    glLineWidth(2);
    DesenhaEixos();

    // Desenha as curvas e pontos
    glLineWidth(3);
    DesenhaPontos();
    DesenhaCurvas();

    // Desenha o rubber-band
    if (nPontoAtual > 0) {
        defineCor(Red);
        
        if (nPontoAtual == 1) {
            DesenhaLinha(PontosClicados[0], PosicaoAtualMouse);
        }
        
        if (nPontoAtual == 2) {
            DesenhaLinha(PontosClicados[0], PontosClicados[1]);
            DesenhaLinha(PontosClicados[1], PosicaoAtualMouse);
        }
    }

    // Desenha as áreas da interface
    DesenhaAreaIcones();
    DesenhaAreaStatus();

    glutSwapBuffers();
}
// **********************************************************************
// ContaTempo(double tempo)
//      conta um certo número de segundos e informa quanto frames
// se passaram neste período.
// **********************************************************************
void ContaTempo(double tempo)
{
    Temporizador T;

    unsigned long cont = 0;
    cout << "Inicio contagem de " << tempo << "segundos ..." << flush;
    while(true)
    {
        tempo -= T.getDeltaT();
        cont++;
        if (tempo <= 0.0)
        {
            cout << "fim! - Passaram-se " << cont << " frames." << endl;
            break;
        }
    }
}
// **********************************************************************
//  void keyboard ( unsigned char key, int x, int y )
// **********************************************************************
void keyboard ( unsigned char key, int x, int y )
{
    switch ( key )
    {
        case 27:        // Termina o programa qdo
            exit ( 0 );   // a tecla ESC for pressionada
            break;
        case 't':
            ContaTempo(3);
            break;
        case ' ':
            desenha = !desenha;
            break;
        case '1':
            if (modoAtual != MODO_SEM_CONTINUIDADE) {
                modoAtual = MODO_SEM_CONTINUIDADE;
                ReiniciaEstado();
                cout << "Modo alterado para: Sem Continuidade" << endl;
            }
            break;
        case '2':
            if (modoAtual != MODO_CONTINUIDADE_POSICAO) {
                modoAtual = MODO_CONTINUIDADE_POSICAO;
                ReiniciaEstado();
                cout << "Modo alterado para: Continuidade de Posição" << endl;
            }
            break;
        case '3':
            if (modoAtual != MODO_CONTINUIDADE_DERIVADA) {
                modoAtual = MODO_CONTINUIDADE_DERIVADA;
                ReiniciaEstado();
                cout << "Modo alterado para: Continuidade de Derivada" << endl;
            }
            break;
        default:
            break;
    }
    glutPostRedisplay();
}
// **********************************************************************
//  void arrow_keys ( int a_keys, int x, int y )
// **********************************************************************
void arrow_keys ( int a_keys, int x, int y )
{
	switch ( a_keys )
	{
        case GLUT_KEY_LEFT:
  
            break;
        case GLUT_KEY_RIGHT:

            break;
		case GLUT_KEY_UP:       // Se pressionar UP
			glutFullScreen ( ); // Vai para Full Screen
			break;
	    case GLUT_KEY_DOWN:     // Se pressionar UP
								// Reposiciona a janela
            glutPositionWindow (50,50);
			glutReshapeWindow ( 700, 500 );
			break;
		default:
			break;
	}
}
// **********************************************************************
// Converte as coordenadas do ponto P de coordenadas de tela para
// coordenadas de universo (sistema de referência definido na glOrtho
// (ver função reshape)
// Este código é baseado em http://hamala.se/forums/viewtopic.php?t=20
// **********************************************************************
Ponto ConvertePonto(Ponto P)
{
    GLint viewport[4];
    GLdouble modelview[16],projection[16];
    GLfloat wx=P.x,wy,wz;
    GLdouble ox=0.0,oy=0.0,oz=0.0;
    glGetIntegerv(GL_VIEWPORT,viewport);
    P.y=viewport[3]-P.y;
    wy=P.y;
    glGetDoublev(GL_MODELVIEW_MATRIX,modelview);
    glGetDoublev(GL_PROJECTION_MATRIX,projection);
    glReadPixels(P.x,P.y,1,1,GL_DEPTH_COMPONENT,GL_FLOAT,&wz);
    gluUnProject(wx,wy,wz,modelview,projection,viewport,&ox,&oy,&oz);
    return Ponto(ox,oy,oz);
}
// **********************************************************************
// Captura o clique do botao esquerdo do mouse sobre a área de
// desenho
// **********************************************************************
void Mouse(int button, int state, int x, int y)
{
    if (button != GLUT_LEFT_BUTTON)
        return;

    if (state == GLUT_DOWN) {
        mouseSegurando = true;

        // No modo sem continuidade, sempre precisamos de 3 pontos
        if (modoAtual == MODO_SEM_CONTINUIDADE) {
            if (nPontoAtual == 0) {
                PontosClicados[0] = ConvertePonto(Ponto(x, y, 0));
                nPontoAtual = 1;
            }
            else if (nPontoAtual == 1) {
                PontosClicados[1] = ConvertePonto(Ponto(x, y, 0));
                nPontoAtual = 2;
            }
            else if (nPontoAtual == 2) {
                PontosClicados[2] = ConvertePonto(Ponto(x, y, 0));
                CriaCurvaNoModoAtual();
                nPontoAtual = 0;
            }
        }
        // Nos modos com continuidade
        else {
            if (primeiraCurva) {
                // Para a primeira curva, precisamos de 3 pontos
                if (nPontoAtual == 0) {
                    PontosClicados[0] = ConvertePonto(Ponto(x, y, 0));
                    nPontoAtual = 1;
                }
                else if (nPontoAtual == 1) {
                    PontosClicados[1] = ConvertePonto(Ponto(x, y, 0));
                    nPontoAtual = 2;
                }
                else if (nPontoAtual == 2) {
                    PontosClicados[2] = ConvertePonto(Ponto(x, y, 0));
                    CriaCurvaNoModoAtual();
                    nPontoAtual = 1; // Próxima curva precisa de 2 pontos
                    PontosClicados[0] = Curvas[nCurvas-1].getPontoFinal();
                }
            }
            else {
                // Para as curvas subsequentes
                if (nPontoAtual == 1) {
                    if (modoAtual == MODO_CONTINUIDADE_POSICAO) {
                        PontosClicados[1] = ConvertePonto(Ponto(x, y, 0));
                        nPontoAtual = 2;
                    }
                    else { // MODO_CONTINUIDADE_DERIVADA
                        PontosClicados[2] = ConvertePonto(Ponto(x, y, 0));
                        CriaCurvaNoModoAtual();
                        nPontoAtual = 1;
                        PontosClicados[0] = Curvas[nCurvas-1].getPontoFinal();
                    }
                }
                else if (nPontoAtual == 2) {
                    PontosClicados[2] = ConvertePonto(Ponto(x, y, 0));
                    CriaCurvaNoModoAtual();
                    nPontoAtual = 1;
                    PontosClicados[0] = Curvas[nCurvas-1].getPontoFinal();
                }
            }
        }
    }

    glutPostRedisplay();
}

// **********************************************************************
// Captura a posição do mouse mesmo quando não está pressionado
// **********************************************************************
void PassiveMotion(int x, int y)
{
    Ponto P(x, y);
    PosicaoAtualMouse = ConvertePonto(P);
    glutPostRedisplay();
}

// **********************************************************************
// Captura as coordenadas do mouse do mouse sobre area de
// desenho, enquanto um dos botoes esta sendo pressionado
// **********************************************************************
void Motion(int x, int y)
{
    Ponto P(x,y);
    PosAtualDoMouse = ConvertePonto(P);
    
    // Atualiza a posição atual do mouse para o rubber-band
    PosicaoAtualMouse = ConvertePonto(P);

    if (aguardandoCliqueFinalP2) {
        PontosClicados[2] = PosAtualDoMouse;
    }

    glutPostRedisplay();
}

// **********************************************************************
//  void main ( int argc, char** argv )
//
// **********************************************************************
int  main ( int argc, char** argv )
{
    cout << "Programa OpenGL" << endl;

    glutInit            ( &argc, argv );
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB );
    glutInitWindowPosition (0,0);

    // Define o tamanho inicial da janela grafica do programa
    glutInitWindowSize  ( 650, 500);

    // Cria a janela na tela, definindo o nome da
    // que aparecera na barra de tútulo da janela.
    glutCreateWindow    ( "Animacao com Bezier" );

    // executa algumas inicializações
    init ();

    // Define que o tratador de evento para
    // o redesenho da tela. A funcao "display"
    // será chamada automaticamente quando
    // for necessário redesenhar a janela
    glutDisplayFunc ( display );

    // Define que o tratador de evento para
    // o invalidação da tela. A funcao "display"
    // será chamada automaticamente sempre que a
    // máquina estiver ociosa (idle)
    glutIdleFunc(animate);

    // Define que o tratador de evento para
    // o redimensionamento da janela. A funcao "reshape"
    // será chamada automaticamente quando
    // o usuário alterar o tamanho da janela
    glutReshapeFunc ( reshape );

    // Define que o tratador de evento para
    // as teclas. A funcao "keyboard"
    // será chamada automaticamente sempre
    // o usuário pressionar uma tecla comum
    glutKeyboardFunc ( keyboard );

    // Define que o tratador de evento para
    // as teclas especiais(F1, F2,... ALT-A,
    // ALT-B, Teclas de Seta, ...).
    // A funcao "arrow_keys" será chamada
    // automaticamente sempre o usuário
    // pressionar uma tecla especial
    glutSpecialFunc ( arrow_keys );
    glutMouseFunc(Mouse);
    glutMotionFunc(Motion);
    glutPassiveMotionFunc(PassiveMotion); // Adiciona o callback para movimento passivo do mouse
    
    // Inicializa a posição do mouse
    PosicaoAtualMouse = Ponto(0, 0, 0);
    
    // inicia o tratamento dos eventos
    glutMainLoop ( );

    return 0;
}
