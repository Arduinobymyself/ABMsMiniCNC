/* 
 Mini CNC, based in TinyCNC https://github.com/MakerBlock/TinyCNC-Sketches
 Send GCODE to this Sketch using gctrl.pde https://github.com/damellis/gctrl
 Convert SVG to GCODE with MakerBot Unicorn plugin for Inkscape available here https://github.com/martymcguire/inkscape-unicorn
 
 ArduinoByMyself
 arduinobymyselg@gmail.com
 arduinobymyself.blogspot.com.br
 */

#include <Servo.h>
#include <Stepper.h>

#define LINE_BUFFER_LENGTH 512

// Configuraçao do servo para as posiçoes
// Up=cima, Down=Baixo
const int penZUp = 50;
const int penZDown = 30;

// Servo no Pino 10
const int penServoPin = 6;

// Passos por revolução
const int stepsPerRevolution = 20; 

// Objeto do Servo
Servo penServo;  

// Inicia motores para eixos X e Y
Stepper myStepperX(stepsPerRevolution, 2,3,4,5);            
Stepper myStepperY(stepsPerRevolution, 8,9,10,11);  

/* Estruturas, variaveis globais    */
struct point { 
  float x; 
  float y; 
  float z; 
};

// Posição atual da cabeça de impressão
struct point actuatorPos;

//  Configurações de desenho
float StepInc = 1;
int StepDelay = 0;
int LineDelay = 50;
int penDelay = 50;

// Passos do motor para andar 1 milimetro
float StepsPerMillimeterX = 6.25;
float StepsPerMillimeterY = 6.25;

// Limites da área de impressão
float Xmin = 0;
float Xmax = 40;
float Ymin = 0;
float Ymax = 40;
float Zmin = 0;
float Zmax = 1;

float Xpos = Xmin;
float Ypos = Ymin;
float Zpos = Zmax; 

// Configurar true para depurar saída
boolean verbose = false;

/**********************
 * void setup() - Iniciações
 ***********************/
void setup() {
  //  Setup
  Serial.begin( 9600 );
  
  penServo.attach(penServoPin);
  penServo.write(penZUp);
  delay(200);

  // Velocidade dos motores
  myStepperX.setSpeed(100);
  myStepperY.setSpeed(100);  

  //  Notificações
  Serial.println("CNC Educacional iniciada com sucesso!");
  Serial.print("X vai de "); 
  Serial.print(Xmin); 
  Serial.print(" para "); 
  Serial.print(Xmax); 
  Serial.println(" mm."); 
  Serial.print("Y vai de "); 
  Serial.print(Ymin); 
  Serial.print(" para "); 
  Serial.print(Ymax); 
  Serial.println(" mm."); 
}

/**********************
 * void loop() - Main loop
 ***********************/
void loop() 
{
  delay(200);
  char line[ LINE_BUFFER_LENGTH ];
  char c;
  int lineIndex;
  bool lineIsComment, lineSemiColon;

  lineIndex = 0;
  lineSemiColon = false;
  lineIsComment = false;

  while (1) {

    // Recepção serial
    while ( Serial.available()>0 ) {
      c = Serial.read();
      if (( c == '\n') || (c == '\r') ) {             // Fim da linha atingido
        if ( lineIndex > 0 ) {                        // Linha completa, executar!
          line[ lineIndex ] = '\0';                   // Terminador da string
          if (verbose) { 
            Serial.print( "Recebido : "); 
            Serial.println( line ); 
          }
          processIncomingLine( line, lineIndex );
          lineIndex = 0;
        } 
        else { 
          // Linha vazia de comentario, pular bloco.
        }
        lineIsComment = false;
        lineSemiColon = false;
        Serial.println("ok");    
      } 
      else {
        if ( (lineIsComment) || (lineSemiColon) ) {   // Joga fora todos os caracteres de comentários
          if ( c == ')' )  lineIsComment = false;     // Fim do comentario, resume linha.
        } 
        else {
          if ( c <= ' ' ) {                           // Joga fora espaços e caracteres de controle
          } 
          else if ( c == '/' ) {                    // Ignorar caracter.
          } 
          else if ( c == '(' ) {                    // Ativa flag de comentarios e ignora todos caracteres até ')' ou EOL.
            lineIsComment = true;
          } 
          else if ( c == ';' ) {
            lineSemiColon = true;
          } 
          else if ( lineIndex >= LINE_BUFFER_LENGTH-1 ) {
            Serial.println( "ERRO - estouro de lineBuffer" );
            lineIsComment = false;
            lineSemiColon = false;
          } 
          else if ( c >= 'a' && c <= 'z' ) {        // Capitaliza caixa-baixa
            line[ lineIndex++ ] = c-'a'+'A';
          } 
          else {
            line[ lineIndex++ ] = c;
          }
        }
      }
    }
  }
}

void processIncomingLine( char* line, int charNB ) {
  int currentIndex = 0;
  char buffer[ 64 ];                                 // Assume 64 como suficiente para 1 parametro
  struct point newPos;

  newPos.x = 0.0;
  newPos.y = 0.0;
  
  while( currentIndex < charNB ) {
    switch ( line[ currentIndex++ ] ) {              // Seleciona comando, se existe
    case 'U':
      penUp(); 
      break;
    case 'D':
      penDown(); 
      break;
    case 'G':
      buffer[0] = line[ currentIndex++ ];          // /!\ Melhorar - Funciona apenas com comandos de 2 digitos
      //      buffer[1] = line[ currentIndex++ ];
      //      buffer[2] = '\0';
      buffer[1] = '\0';

      switch ( atoi( buffer ) ){                   // Seleciona comando G
      case 0:                                   // G00 & G01 - Move ou move rapido. Da no mesmo
      case 1:
        // /!\ Melhorar - Assume que X está antes de Y
        char* indexX = strchr( line+currentIndex, 'X' );  // Captura posição x/Y na string se existe
        char* indexY = strchr( line+currentIndex, 'Y' );
        if ( indexY <= 0 ) {
          newPos.x = atof( indexX + 1); 
          newPos.y = actuatorPos.y;
        } 
        else if ( indexX <= 0 ) {
          newPos.y = atof( indexY + 1);
          newPos.x = actuatorPos.x;
        } 
        else {
          newPos.y = atof( indexY + 1);
          indexY = '\0';
          newPos.x = atof( indexX + 1);
        }
        drawLine(newPos.x, newPos.y );
        //        Serial.println("ok");
        actuatorPos.x = newPos.x;
        actuatorPos.y = newPos.y;
        break;
      }
      break;
    case 'M':
      buffer[0] = line[ currentIndex++ ];        // /!\ Melhorar - Só funciona com comandos de 3 digitos
      buffer[1] = line[ currentIndex++ ];
      buffer[2] = line[ currentIndex++ ];
      buffer[3] = '\0';
      switch ( atoi( buffer ) ){
      case 300:
        {
          char* indexS = strchr( line+currentIndex, 'S' );
          float Spos = atof( indexS + 1);
          //          Serial.println("ok");
          if (Spos == 30) { 
            penDown(); 
          }
          if (Spos == 50) { 
            penUp(); 
          }
          break;
        }
      case 114:                                // M114 - Reporta posição
        Serial.print( "Posicao absoluta : X = " );
        Serial.print( actuatorPos.x );
        Serial.print( "  -  Y = " );
        Serial.println( actuatorPos.y );
        break;
      default:
        Serial.print( "Comando nao reconhecido: M");
        Serial.println( buffer );
      }
    }
  }



}


/*********************************
 * Desenha linha de (x0;y0) para (x1;y1). 
 * Bresenham algoritimo de https://www.marginallyclever.com/blog/2013/08/how-to-build-an-2-axis-arduino-cnc-gcode-interpreter/
 * int (x1;y1) : Coordenadas iniciais
 * int (x2;y2) : Coordenadas finais
 **********************************/
void drawLine(float x1, float y1) {

  if (verbose)
  {
    Serial.print("fx1, fy1: ");
    Serial.print(x1);
    Serial.print(",");
    Serial.print(y1);
    Serial.println("");
  }  

  //  Atribui limites para instruções
  if (x1 >= Xmax) { 
    x1 = Xmax; 
  }
  if (x1 <= Xmin) { 
    x1 = Xmin; 
  }
  if (y1 >= Ymax) { 
    y1 = Ymax; 
  }
  if (y1 <= Ymin) { 
    y1 = Ymin; 
  }

  if (verbose)
  {
    Serial.print("Xpos, Ypos: ");
    Serial.print(Xpos);
    Serial.print(",");
    Serial.print(Ypos);
    Serial.println("");
  }

  if (verbose)
  {
    Serial.print("x1, y1: ");
    Serial.print(x1);
    Serial.print(",");
    Serial.print(y1);
    Serial.println("");
  }

  //  Converte coordenadas para passos
  x1 = (int)(x1*StepsPerMillimeterX);
  y1 = (int)(y1*StepsPerMillimeterY);
  float x0 = Xpos;
  float y0 = Ypos;

  //  Vamos encontrar o sentido das coordenadas
  long dx = abs(x1-x0);
  long dy = abs(y1-y0);
  int sx = x0<x1 ? StepInc : -StepInc;
  int sy = y0<y1 ? StepInc : -StepInc;

  long i;
  long over = 0;

  if (dx > dy) {
    for (i=0; i<dx; ++i) {
      myStepperX.step(sx);
      over+=dy;
      if (over>=dx) {
        over-=dx;
        myStepperY.step(sy);
      }
      delay(StepDelay);
    }
  }
  else {
    for (i=0; i<dy; ++i) {
      myStepperY.step(sy);
      over+=dx;
      if (over>=dy) {
        over-=dy;
        myStepperX.step(sx);
      }
      delay(StepDelay);
    }    
  }

  if (verbose)
  {
    Serial.print("dx, dy:");
    Serial.print(dx);
    Serial.print(",");
    Serial.print(dy);
    Serial.println("");
  }

  if (verbose)
  {
    Serial.print("Indo para (");
    Serial.print(x0);
    Serial.print(",");
    Serial.print(y0);
    Serial.println(")");
  }

  //  Atraso de tempo antes das próximas linhas serem enviadas
  delay(LineDelay);
  //  Atualizando posições
  Xpos = x1;
  Ypos = y1;
}

//  Levanta caneta
void penUp() { 
  penServo.write(penZUp); 
  delay(LineDelay); 
  Zpos=Zmax; 
  if (verbose) { 
    Serial.println("Levanta caneta!"); 
  } 
}
//  Abaixa caneta
void penDown() { 
  penServo.write(penZDown); 
  delay(LineDelay); 
  Zpos=Zmin; 
  if (verbose) { 
    Serial.println("Abaixa caneta."); 
  } 
  delay(300);
}
