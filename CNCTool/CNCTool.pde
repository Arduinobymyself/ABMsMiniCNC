import java.awt.event.KeyEvent;
import javax.swing.JOptionPane;
import processing.serial.*;

Serial port = null;

// Selecione e modifique as linhas abaixo conforme seu sistema operacional
// ou deixe null para usar o modo interativo de seleção de porta 
// (pressione 'p' no programa)
String portname = null;
//String portname = Serial.list()[0]; // Mac OS X
//String portname = "/dev/ttyUSB0"; // Linux
//String portname = "COM6"; // Windows

boolean streaming = false;
float speed = 0.001;
String[] gcode;
int i = 0;

void openSerialPort()
{
  if (portname == null) return;
  if (port != null) port.stop();
  
  port = new Serial(this, portname, 9600);
  
  port.bufferUntil('\n');
}

void selectSerialPort()
{
  String result = (String) JOptionPane.showInputDialog(frame,
    "Selecione a porta serial que corresponde ao seu Arduino.",
    "Selecione a porta serial",
    JOptionPane.QUESTION_MESSAGE,
    null,
    Serial.list(),
    0);
    
  if (result != null) {
    portname = result;
    openSerialPort();
  }
}

void setup()
{
  size(500, 250);
  openSerialPort();
}

void draw()
{
  background(0);  
  fill(255);
  int y = 24, dy = 12;
  text("INSTRUCOES", 12, y); y += dy;
  text("p: seleciona porta serial", 12, y); y += dy;
  text("1: configura velocidade para 0.001 polegadas por passo", 12, y); y += dy;
  text("2: configura velocidade para 0.010 polegadas por passo", 12, y); y += dy;
  text("3: configura velocidade para 0.100 polegadas por passo", 12, y); y += dy;
  text("setas: efetua passo no plano x-y", 12, y); y += dy;
  text("page up & page down: efetua passo no plano z", 12, y); y += dy;
  text("$: mostra configuracoes grbl", 12, y); y+= dy;
  text("h: posicao inicial", 12, y); y += dy;
  text("0: zerar maquina (configura posicao atual como inicial)", 12, y); y += dy;
  text("g: envia arquivo g-code", 12, y); y += dy;
  text("x: para de enviar arquivo g-code (pode demorar)", 12, y); y += dy;
  y = height - dy;
  text("velocidade atual: " + speed + " polegadas por passos", 12, y); y -= dy;
  text("porta serial atual: " + portname, 12, y); y -= dy;
}

void keyPressed()
{
  if (key == '1') speed = 0.001;
  if (key == '2') speed = 0.01;
  if (key == '3') speed = 0.1;
  
  if (!streaming) {
    if (keyCode == LEFT) port.write("G91\nG20\nG00 X-" + speed + " Y0.000 Z0.000\n");
    if (keyCode == RIGHT) port.write("G91\nG20\nG00 X" + speed + " Y0.000 Z0.000\n");
    if (keyCode == UP) port.write("G91\nG20\nG00 X0.000 Y" + speed + " Z0.000\n");
    if (keyCode == DOWN) port.write("G91\nG20\nG00 X0.000 Y-" + speed + " Z0.000\n");
    if (keyCode == KeyEvent.VK_PAGE_UP) port.write("G91\nG20\nG00 X0.000 Y0.000 Z" + speed + "\n");
    if (keyCode == KeyEvent.VK_PAGE_DOWN) port.write("G91\nG20\nG00 X0.000 Y0.000 Z-" + speed + "\n");
    if (key == 'h') port.write("G90\nG20\nG00 X0.000 Y0.000 Z0.000\n");
    if (key == 'v') port.write("$0=75\n$1=74\n$2=75\n");
    //if (key == 'v') port.write("$0=100\n$1=74\n$2=75\n");
    if (key == 's') port.write("$3=10\n");
    if (key == 'e') port.write("$16=1\n");
    if (key == 'd') port.write("$16=0\n");
    if (key == '0') openSerialPort();
    if (key == 'p') selectSerialPort();
    if (key == '$') port.write("$$\n");
  }
  
  if (!streaming && key == 'g') {
    gcode = null; i = 0;
    File file = null; 
    println("Carregando arquivo...");
    selectInput("Selecione um arquivo para processar:", "fileSelected", file);
  }
  
  if (key == 'x') streaming = false;
}

void fileSelected(File selection) {
  if (selection == null) {
    println("Janela fechada ou cancelada pelo usuario.");
  } else {
    println("Usuario selecionou " + selection.getAbsolutePath());
    gcode = loadStrings(selection.getAbsolutePath());
    if (gcode == null) return;
    streaming = true;
    stream();
  }
}

void stream()
{
  if (!streaming) return;
  
  while (true) {
    if (i == gcode.length) {
      streaming = false;
      return;
    }
    
    if (gcode[i].trim().length() == 0) i++;
    else break;
  }
  
  println(gcode[i]);
  port.write(gcode[i] + '\n');
  i++;
}

void serialEvent(Serial p)
{
  String s = p.readStringUntil('\n');
  println(s.trim());
  
  if (s.trim().startsWith("ok")) stream();
  if (s.trim().startsWith("erro")) stream(); // XXX: really?
}
