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

#include <iostream>
#include <cmath>
#include <ctime>
#include <fstream>
#include <vector>
#include <algorithm>  // Necessário para remove_if

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

#define MAX 100  // Definindo a constante MAX

// Modos de edição
enum ModoEdicao {
    MODO_CRIAR = 0,
    MODO_MOVER_VERTICE = 1,
    MODO_REMOVER_CURVA = 2,
    MODO_CONECTAR_CURVA = 3,
    MODO_ALTERAR_CONTINUIDADE = 4
};

// Adicionando variáveis para os botões
struct Botao {
    float x1, y1, x2, y2;
    string texto;
    ModoEdicao modo;
    bool selecionado;
};

// Declarações antecipadas das funções
float calculaDistanciaEntrePontos(Ponto p1, Ponto p2);
float DistanciaPontoReta(Ponto P, Ponto A, Ponto B);
bool VerificaCliqueCurva(Ponto ponto, int& curvaIndex, int& pontoIndex);
void AtualizaCurvasRelacionadas(int curvaIndex);
void InicializaBotoes();
bool PontoDentroBotao(Ponto p, Botao b);
void DesenhaBotoesEdicao();
string ObterMensagemModo();
void PreservaEstadoAoMudarModo(ModoEdicao novoModo);

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

// Variáveis de controle para edição
ModoEdicao modoEdicao = MODO_CRIAR;
bool exibirCurvas = true;
bool exibirPoligonos = true;
bool exibirVertices = true;
int curvaEmEdicao = -1;
int verticeEmEdicao = -1;

// Nomes dos modos de edição
string NomesModosEdicao[] = {
    "Criar Curva",
    "Mover Vértice",
    "Remover Curva",
    "Conectar Curva",
    "Alterar Continuidade"
};

// Estrutura para armazenar informações de continuidade entre curvas
struct ContinuidadeCurvas {
    int curva1;
    int curva2;
    int tipoContinuidade; // 0: sem continuidade, 1: posição, 2: derivada
};
vector<ContinuidadeCurvas> continuidades;

vector<Botao> botoesEdicao;

// Adicionando variável para rastrear o ponto clicado
Ponto pontoClicado;  // Ponto onde o mouse foi clicado
Ponto Curva[MAX];  // Array para armazenar pontos da curva

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
            nCurvas++;
            break;
            
        case MODO_CONTINUIDADE_POSICAO:
            if (primeiraCurva) {
                Curvas[nCurvas] = Bezier(PontosClicados[0], PontosClicados[1], PontosClicados[2]);
                primeiraCurva = false;
            } else {
                // Usa o último ponto da curva anterior como primeiro ponto
                Ponto p0 = Curvas[nCurvas-1].getPC(2);
                Curvas[nCurvas] = Bezier(p0, PontosClicados[1], PontosClicados[2]);
            }
            nCurvas++;
            break;
            
        case MODO_CONTINUIDADE_DERIVADA:
            if (primeiraCurva) {
                Curvas[nCurvas] = Bezier(PontosClicados[0], PontosClicados[1], PontosClicados[2]);
                primeiraCurva = false;
            } else {
                // Usa o último ponto da curva anterior como primeiro ponto
                Ponto p0 = Curvas[nCurvas-1].getPC(2);
                Ponto p1 = Curvas[nCurvas-1].getPC(1);
                
                // Calcula o ponto de controle para manter a derivada
                Ponto direcao = p0 - p1;
                Ponto p1Novo = p0 + direcao;
                
                Curvas[nCurvas] = Bezier(p0, p1Novo, PontosClicados[2]);
            }
            nCurvas++;
            break;
    }
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
    
    // Inicializa os botões
    InicializaBotoes();
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
    printString("1", 10.7, posY - 1.0, White);  // Movido mais para a esquerda
    printString("Sem Cont.", 11.3, posY - 1.0, White);  // Ajustado para não sobrepor o número
    
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
    printString("2", 10.7, posY - 1.0, White);  // Movido mais para a esquerda
    printString("Cont. Pos.", 11.3, posY - 1.0, White);  // Ajustado para não sobrepor o número
    
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
    printString("3", 10.7, posY - 1.0, White);  // Movido mais para a esquerda
    printString("Cont. Der.", 11.3, posY - 1.0, White);  // Ajustado para não sobrepor o número
    
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

    // Desenha os botões de edição
    DesenhaBotoesEdicao();
    
    // Desenha a mensagem do modo atual
    string mensagem = ObterMensagemModo();
    printString(mensagem, -14, 11, Yellow);

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
// Função para preservar o estado ao mudar de modo
// **********************************************************************
void PreservaEstadoAoMudarModo(ModoEdicao novoModo) {
    // Se estiver no meio de criar uma curva, preserva os pontos
    if (modoEdicao == MODO_CRIAR && nPontoAtual > 0) {
        // Mantém os pontos clicados
        return;
    }
    
    // Se estiver movendo um vértice, finaliza a operação
    if (modoEdicao == MODO_MOVER_VERTICE && curvaEmEdicao != -1) {
        AtualizaCurvasRelacionadas(curvaEmEdicao);
        curvaEmEdicao = -1;
        verticeEmEdicao = -1;
    }
    
    // Reseta o estado de edição
    nPontoAtual = 0;
    curvaEmEdicao = -1;
    verticeEmEdicao = -1;
}

// **********************************************************************
//  void keyboard ( unsigned char key, int x, int y )
// **********************************************************************
void keyboard ( unsigned char key, int x, int y )
{
    switch ( key )
    {
        case 27:     // Tecla "ESC" - encerra o programa
            exit ( 0 );
            break;
        case 'c':
        case 'C':
            PreservaEstadoAoMudarModo(MODO_CRIAR);
            modoEdicao = MODO_CRIAR;
            for (auto& b : botoesEdicao) {
                b.selecionado = (b.modo == MODO_CRIAR);
            }
            break;
        case 'm':
        case 'M':
            PreservaEstadoAoMudarModo(MODO_MOVER_VERTICE);
            modoEdicao = MODO_MOVER_VERTICE;
            for (auto& b : botoesEdicao) {
                b.selecionado = (b.modo == MODO_MOVER_VERTICE);
            }
            break;
        case 'r':
        case 'R':
            PreservaEstadoAoMudarModo(MODO_REMOVER_CURVA);
            modoEdicao = MODO_REMOVER_CURVA;
            for (auto& b : botoesEdicao) {
                b.selecionado = (b.modo == MODO_REMOVER_CURVA);
            }
            break;
        case 'n':
        case 'N':
            PreservaEstadoAoMudarModo(MODO_CONECTAR_CURVA);
            modoEdicao = MODO_CONECTAR_CURVA;
            for (auto& b : botoesEdicao) {
                b.selecionado = (b.modo == MODO_CONECTAR_CURVA);
            }
            break;
        case 'a':
        case 'A':
            PreservaEstadoAoMudarModo(MODO_ALTERAR_CONTINUIDADE);
            modoEdicao = MODO_ALTERAR_CONTINUIDADE;
            for (auto& b : botoesEdicao) {
                b.selecionado = (b.modo == MODO_ALTERAR_CONTINUIDADE);
            }
            break;
        case '1':
            modoAtual = MODO_SEM_CONTINUIDADE;
            primeiraCurva = true;
            break;
        case '2':
            modoAtual = MODO_CONTINUIDADE_POSICAO;
            primeiraCurva = true;
            break;
        case '3':
            modoAtual = MODO_CONTINUIDADE_DERIVADA;
            primeiraCurva = true;
            break;
        case 'v':
        case 'V':
            exibirVertices = !exibirVertices;
            break;
        case 'p':
        case 'P':
            exibirPoligonos = !exibirPoligonos;
            break;
        case 'b':
        case 'B':
            exibirCurvas = !exibirCurvas;
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
    Ponto P = ConvertePonto(Ponto(x, y, 0));
    pontoClicado = P;  // Atualiza o ponto clicado
    
    if (button != GLUT_LEFT_BUTTON)
        return;

    if (state == GLUT_DOWN) {
        mouseSegurando = true;
        
        // Verifica se clicou em algum botão de edição
        for (auto& botao : botoesEdicao) {
            if (PontoDentroBotao(pontoClicado, botao)) {
                // Preserva o estado antes de mudar o modo
                PreservaEstadoAoMudarModo(botao.modo);
                
                // Atualiza o modo de edição
                modoEdicao = botao.modo;
                
                // Atualiza a seleção dos botões
                for (auto& b : botoesEdicao) {
                    b.selecionado = (b.modo == modoEdicao);
                }
                
                glutPostRedisplay();
                return;
            }
        }
        
        // Verifica se clicou nos botões de modo
        float posY = 11;
        float altura = 1.5;
        float espacamento = 2.0;
        
        // Botão modo sem continuidade
        if (pontoClicado.x >= 10.5 && pontoClicado.x <= 14.5 &&
            pontoClicado.y >= posY - altura && pontoClicado.y <= posY) {
            modoAtual = MODO_SEM_CONTINUIDADE;
            primeiraCurva = true;
            glutPostRedisplay();
            return;
        }
        
        // Botão modo continuidade de posição
        posY -= espacamento;
        if (pontoClicado.x >= 10.5 && pontoClicado.x <= 14.5 &&
            pontoClicado.y >= posY - altura && pontoClicado.y <= posY) {
            modoAtual = MODO_CONTINUIDADE_POSICAO;
            primeiraCurva = true;
            glutPostRedisplay();
            return;
        }
        
        // Botão modo continuidade de derivada
        posY -= espacamento;
        if (pontoClicado.x >= 10.5 && pontoClicado.x <= 14.5 &&
            pontoClicado.y >= posY - altura && pontoClicado.y <= posY) {
            modoAtual = MODO_CONTINUIDADE_DERIVADA;
            primeiraCurva = true;
            glutPostRedisplay();
            return;
        }
        
        int curvaIndex, verticeIndex;
        bool clicouEmCurva = VerificaCliqueCurva(pontoClicado, curvaIndex, verticeIndex);
        
        switch(modoEdicao) {
            case MODO_CRIAR:
                if (modoAtual == MODO_SEM_CONTINUIDADE) {
                    if (nPontoAtual == 0) {
                        PontosClicados[0] = pontoClicado;
                        nPontoAtual = 1;
                    }
                    else if (nPontoAtual == 1) {
                        PontosClicados[1] = pontoClicado;
                        nPontoAtual = 2;
                    }
                    else if (nPontoAtual == 2) {
                        PontosClicados[2] = pontoClicado;
                        CriaCurvaNoModoAtual();
                        nPontoAtual = 0;
                    }
                }
                else if (modoAtual == MODO_CONTINUIDADE_POSICAO) {
                    if (primeiraCurva) {
                        // Primeira curva no modo de continuidade por posição
                        if (nPontoAtual == 0) {
                            PontosClicados[0] = pontoClicado;
                            nPontoAtual = 1;
                        }
                        else if (nPontoAtual == 1) {
                            PontosClicados[1] = pontoClicado;
                            nPontoAtual = 2;
                        }
                        else if (nPontoAtual == 2) {
                            PontosClicados[2] = pontoClicado;
                            CriaCurvaNoModoAtual();
                            nPontoAtual = 0;
                        }
                    } else {
                        // Continuando a partir da última curva
                        if (nPontoAtual == 0) {
                            // O primeiro ponto é o último ponto da curva anterior
                            PontosClicados[0] = Curvas[nCurvas-1].getPC(2);
                            nPontoAtual = 1;
                        }
                        else if (nPontoAtual == 1) {
                            PontosClicados[1] = pontoClicado;
                            nPontoAtual = 2;
                        }
                        else if (nPontoAtual == 2) {
                            PontosClicados[2] = pontoClicado;
                            CriaCurvaNoModoAtual();
                            nPontoAtual = 0;
                        }
                    }
                }
                else if (modoAtual == MODO_CONTINUIDADE_DERIVADA) {
                    if (primeiraCurva) {
                        // Primeira curva no modo de continuidade por derivada
                        if (nPontoAtual == 0) {
                            PontosClicados[0] = pontoClicado;
                            nPontoAtual = 1;
                        }
                        else if (nPontoAtual == 1) {
                            PontosClicados[1] = pontoClicado;
                            nPontoAtual = 2;
                        }
                        else if (nPontoAtual == 2) {
                            PontosClicados[2] = pontoClicado;
                            CriaCurvaNoModoAtual();
                            nPontoAtual = 0;
                        }
                    } else {
                        // Continuando a partir da última curva
                        if (nPontoAtual == 0) {
                            // O primeiro ponto é o último ponto da curva anterior
                            PontosClicados[0] = Curvas[nCurvas-1].getPC(2);
                            nPontoAtual = 1;
                        }
                        else if (nPontoAtual == 1) {
                            // O segundo ponto é calculado para manter a derivada
                            Ponto p0 = Curvas[nCurvas-1].getPC(2);
                            Ponto p1 = Curvas[nCurvas-1].getPC(1);
                            Ponto direcao = p0 - p1;
                            PontosClicados[1] = p0 + direcao;
                            nPontoAtual = 2;
                        }
                        else if (nPontoAtual == 2) {
                            PontosClicados[2] = pontoClicado;
                            CriaCurvaNoModoAtual();
                            nPontoAtual = 0;
                        }
                    }
                }
                break;
                
            case MODO_MOVER_VERTICE:
                if(clicouEmCurva && verticeIndex != -1) {
                    curvaEmEdicao = curvaIndex;
                    verticeEmEdicao = verticeIndex;
                }
                break;
                
            case MODO_REMOVER_CURVA:
                if(clicouEmCurva && exibirPoligonos) {
                    // Remove a curva
                    for(int i = curvaIndex; i < nCurvas-1; i++) {
                        Curvas[i] = Curvas[i+1];
                    }
                    nCurvas--;
                    
                    // Atualiza as continuidades
                    continuidades.erase(
                        remove_if(continuidades.begin(), continuidades.end(),
                            [curvaIndex](const ContinuidadeCurvas& c) {
                                return c.curva1 == curvaIndex || c.curva2 == curvaIndex;
                            }
                        ),
                        continuidades.end()
                    );
                }
                break;
                
            case MODO_CONECTAR_CURVA:
                if(clicouEmCurva && (verticeIndex == 0 || verticeIndex == 2)) {
                    // Inicia uma nova curva a partir do vértice selecionado
                    PontosClicados[0] = Curvas[curvaIndex].getPC(verticeIndex);
                    nPontoAtual = 1;
                    curvaEmEdicao = curvaIndex;
                    verticeEmEdicao = verticeIndex;
                }
                break;
                
            case MODO_ALTERAR_CONTINUIDADE:
                if(clicouEmCurva && verticeIndex == 2) {
                    // Procura se já existe continuidade com próxima curva
                    auto it = find_if(continuidades.begin(), continuidades.end(),
                        [curvaIndex](const ContinuidadeCurvas& c) {
                            return c.curva1 == curvaIndex;
                        }
                    );
                    
                    if(it != continuidades.end()) {
                        // Aumenta o grau de continuidade ou remove se já estiver no máximo
                        if(it->tipoContinuidade == 2)
                            continuidades.erase(it);
                        else
                            it->tipoContinuidade++;
                    }
                    else if(curvaIndex < nCurvas-1) {
                        // Cria nova continuidade
                        continuidades.push_back({curvaIndex, curvaIndex+1, 1});
                    }
                    AtualizaCurvasRelacionadas(curvaIndex);
                }
                break;
        }
    }
    else if (state == GLUT_UP) {
        mouseSegurando = false;
        if(modoEdicao == MODO_MOVER_VERTICE && curvaEmEdicao != -1) {
            AtualizaCurvasRelacionadas(curvaEmEdicao);
            curvaEmEdicao = -1;
            verticeEmEdicao = -1;
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
    Ponto P = ConvertePonto(Ponto(x,y));
    pontoClicado = P;  // Atualiza o ponto clicado
    PosAtualDoMouse = P;
    PosicaoAtualMouse = P;
    
    if(modoEdicao == MODO_MOVER_VERTICE && curvaEmEdicao != -1) {
        Curvas[curvaEmEdicao].setPC(verticeEmEdicao, PosicaoAtualMouse);
    }
    
    glutPostRedisplay();
}

// **********************************************************************
// Funções de edição
// **********************************************************************
bool VerificaCliqueCurva(Ponto ponto, int& curvaIndex, int& pontoIndex)
{
    const float TOLERANCIA = 5.0; // Tolerância em pixels para detecção de clique
    
    // Verifica clique nos pontos de controle
    for(int i = 0; i < nCurvas; i++) {
        for(int j = 0; j < 3; j++) {  // Uma curva de Bezier tem 3 pontos de controle
            if(calculaDistanciaEntrePontos(ponto, Curvas[i].getPC(j)) < TOLERANCIA) {
                curvaIndex = i;
                pontoIndex = j;
                return true;
            }
        }
    }
    
    // Verifica clique nas arestas do polígono de controle
    for(int i = 0; i < nCurvas; i++) {
        for(int j = 0; j < 2; j++) {  // 2 arestas no polígono de controle
            if(DistanciaPontoReta(ponto, Curvas[i].getPC(j), Curvas[i].getPC(j+1)) < TOLERANCIA &&
               ponto.x >= min(Curvas[i].getPC(j).x, Curvas[i].getPC(j+1).x) - TOLERANCIA &&
               ponto.x <= max(Curvas[i].getPC(j).x, Curvas[i].getPC(j+1).x) + TOLERANCIA &&
               ponto.y >= min(Curvas[i].getPC(j).y, Curvas[i].getPC(j+1).y) - TOLERANCIA &&
               ponto.y <= max(Curvas[i].getPC(j).y, Curvas[i].getPC(j+1).y) + TOLERANCIA) {
                curvaIndex = i;
                pontoIndex = j;
                return true;
            }
        }
    }
    
    return false;
}

void AtualizaCurvasRelacionadas(int curvaIndex)
{
    // Atualiza as curvas que têm continuidade com a curva modificada
    for(auto& cont : continuidades) {
        if(cont.curva1 == curvaIndex) {
            if(cont.tipoContinuidade == 1) { // Continuidade de posição
                Curvas[cont.curva2].setPC(0, Curvas[curvaIndex].getPontoFinal());
            }
            else if(cont.tipoContinuidade == 2) { // Continuidade de derivada
                Ponto p0 = Curvas[curvaIndex].getPontoFinal();
                Ponto dir = Curvas[curvaIndex].getDirecaoFinal();
                Curvas[cont.curva2].setPC(0, p0);
                Curvas[cont.curva2].setPC(1, p0 + dir);
            }
        }
    }
}

// **********************************************************************
// Funções auxiliares
// **********************************************************************
float calculaDistanciaEntrePontos(Ponto p1, Ponto p2)
{
    return sqrt(pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2));
}

float DistanciaPontoReta(Ponto P, Ponto A, Ponto B)
{
    // Calcula a distância de um ponto P a uma reta definida por dois pontos A e B
    float numerador = fabs((B.y - A.y) * P.x - (B.x - A.x) * P.y + B.x * A.y - B.y * A.x);
    float denominador = sqrt(pow(B.y - A.y, 2) + pow(B.x - A.x, 2));
    
    if(denominador == 0) return calculaDistanciaEntrePontos(P, A);
    return numerador / denominador;
}

// **********************************************************************
// Inicializa os botões de edição
// **********************************************************************
void InicializaBotoes() {
    float largura = 3.5;  // Aumentado de 3.0 para 3.5
    float altura = 1.2;
    float espacamento = 0.7;  // Aumentado de 0.5 para 0.7
    float posX = -14.0;
    float posY = 13.0;
    
    // Botão Criar
    botoesEdicao.push_back({posX, posY, posX + largura, posY - altura, "Criar (c)", MODO_CRIAR, true});
    
    // Botão Mover Vértice
    posX += largura + espacamento;
    botoesEdicao.push_back({posX, posY, posX + largura, posY - altura, "Mover (m)", MODO_MOVER_VERTICE, false});
    
    // Botão Remover Curva
    posX += largura + espacamento;
    botoesEdicao.push_back({posX, posY, posX + largura, posY - altura, "Remover (r)", MODO_REMOVER_CURVA, false});
    
    // Botão Conectar Curva
    posX += largura + espacamento;
    botoesEdicao.push_back({posX, posY, posX + largura, posY - altura, "Conectar (n)", MODO_CONECTAR_CURVA, false});
    
    // Botão Alterar Continuidade
    posX += largura + espacamento;
    botoesEdicao.push_back({posX, posY, posX + largura, posY - altura, "Cont. (a)", MODO_ALTERAR_CONTINUIDADE, false});
}

// Verifica se um ponto está dentro de um botão
bool PontoDentroBotao(Ponto p, Botao b) {
    return (p.x >= b.x1 && p.x <= b.x2 && p.y >= b.y1 && p.y <= b.y2);
}

// Desenha os botões de edição
void DesenhaBotoesEdicao() {
    for (auto& botao : botoesEdicao) {
        // Desenha o fundo do botão
        if (botao.selecionado) {
            glColor3f(0.8, 0.2, 0.2); // Vermelho quando selecionado
        } else {
            glColor3f(0.4, 0.4, 0.4); // Cinza quando não selecionado
        }
        
        glBegin(GL_QUADS);
        glVertex2f(botao.x1, botao.y1);
        glVertex2f(botao.x2, botao.y1);
        glVertex2f(botao.x2, botao.y2);
        glVertex2f(botao.x1, botao.y2);
        glEnd();
        
        // Desenha a borda do botão
        glColor3f(1.0, 1.0, 1.0);
        glBegin(GL_LINE_LOOP);
        glVertex2f(botao.x1, botao.y1);
        glVertex2f(botao.x2, botao.y1);
        glVertex2f(botao.x2, botao.y2);
        glVertex2f(botao.x1, botao.y2);
        glEnd();
        
        // Desenha o texto do botão
        glColor3f(1.0, 1.0, 1.0);
        float centroX = (botao.x1 + botao.x2) / 2;
        float centroY = (botao.y1 + botao.y2) / 2;
        // Ajustando o posicionamento do texto para ficar mais centralizado
        float offsetX = botao.texto.length() * 0.15; // Ajuste baseado no tamanho do texto
        printString(botao.texto, centroX - offsetX, centroY - 0.1, White);
    }
}

// Atualiza as mensagens baseadas no modo atual
string ObterMensagemModo() {
    switch (modoEdicao) {
        case MODO_CRIAR:
            if (nPontoAtual < 3) {
                return Mensagens[nPontoAtual];
            } else {
                return "Clique para reiniciar.";
            }
        case MODO_MOVER_VERTICE:
            return "Clique e arraste um vértice para movê-lo.";
        case MODO_REMOVER_CURVA:
            return "Clique em uma curva para removê-la.";
        case MODO_CONECTAR_CURVA:
            return "Clique em um ponto de controle para conectar.";
        case MODO_ALTERAR_CONTINUIDADE:
            return "Clique no ponto final de uma curva para alterar a continuidade.";
        default:
            return "";
    }
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


