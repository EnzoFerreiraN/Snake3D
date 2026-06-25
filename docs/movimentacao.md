# Movimentação

## Grid lógico e estruturas de dados

A arena do jogo é um grid bidimensional de **30 × 30 células** (`#define GRID 30`), cada célula com tamanho `1.0f` em unidades OpenGL (`#define CELL 1.0f`). A posição de cada elemento do jogo é armazenada em coordenadas de grid (inteiros), não em coordenadas do mundo. A conversão de grid para mundo é feita pelas funções auxiliares inline:

```cpp
float gx(int x) { return (x - GRID * 0.5f + 0.5f) * CELL; }
float gz(int z) { return (z - GRID * 0.5f + 0.5f) * CELL; }
```

Isso centraliza o grid na origem do mundo e permite que a câmera aponte para `(0, 0, 0)`.

Duas structs simples representam os elementos fundamentais:

```cpp
struct Cell { int x, z; };   // posição no grid
struct Dir  { int dx, dz; }; // deslocamento por passo (±1 em x ou z)
```

`Dir` também define `isOpposite()` para verificar se uma direção é o oposto da atual — usado para impedir que a cobra inverta o sentido de marcha em 180°.

A cobra é armazenada como `std::vector<Cell> snake`, onde o índice `0` é sempre a **cabeça**. O grid lógico em `int gridState[GRID][GRID]` armazena se cada célula está vazia (`CT_EMPTY`) ou tem parede (`CT_WALL`).

---

## Direção: atual vs. próxima

O jogo distingue duas variáveis de direção:

- `curDir` — direção que está sendo executada no passo atual
- `nextDir` — direção que será aplicada no próximo passo

Quando o jogador pressiona uma tecla, a função de input atualiza `nextDir`, não `curDir`. No início de cada `step()`, `curDir` recebe o valor de `nextDir`. Isso evita que dois toques rápidos consecutivos façam a cobra colidir consigo mesma ao "pular" uma direção intermediária.

A validação de inversão acontece no momento do input:

```cpp
Dir d; d.dx = 0; d.dz = -1;          // nova direção desejada
if (!d.isOpposite(curDir)) nextDir = d; // só aplica se não for inversão
```

---

## Passo discreto (`step`)

O movimento da cobra é **discreto**: a cada passo ela avança exatamente uma célula. A função `step()` executa a seguinte sequência:

1. Aplica `nextDir` em `curDir`.
2. Calcula a nova posição da cabeça: `head.x += curDir.dx; head.z += curDir.dz`.
3. Verifica teleporte por portal (`checkPortal`).
4. Testa colisões (borda do grid, parede, auto-colisão) → se colidir, `gameOver = true`.
5. Insere a nova cabeça no início do vetor: `snake.insert(snake.begin(), head)`.
6. Se a cabeça está na célula da comida: **não remove a cauda** (a cobra cresce). Caso contrário, remove o último elemento: `snake.pop_back()`.

Esse mecanismo simples — inserir na frente, remover do fundo — implementa o crescimento e o movimento do Snake sem nenhuma estrutura de dados adicional.

---

## Avanço por tempo

O passo não é executado a cada frame, mas em intervalos de tempo controlados pela função de timer do GLUT:

```cpp
static void timer(int) {
    step();
    if (!gameOver)
        glutTimerFunc((unsigned int)stepMs, timer, 0);
    glutPostRedisplay();
}
```

`stepMs` começa em **150 ms** (`STEP_MS_INIT`) e é reduzido em **5 ms** (`SPEED_INCR`) a cada maçã comida, até o mínimo de **60 ms** (`STEP_MS_MIN`). Isso cria a aceleração progressiva do jogo. O timer se re-agenda a si mesmo com o valor atualizado de `stepMs`, ajustando automaticamente a velocidade. Ao ocorrer game over, o timer para de se re-agendar.

---

## Input de teclado e mouse

### Teclado — `keyboard` e `specialKeys`

Duas callbacks GLUT tratam o input:

- `keyboard(unsigned char key, ...)` — teclas ASCII: WASD para direção, C para câmera, M para música, R para reiniciar, ESC para sair.
- `specialKeys(int key, ...)` — teclas especiais: `GLUT_KEY_UP/DOWN/LEFT/RIGHT` (setas do teclado), que mapeiam para as quatro direções do grid.

Ambas as funções constroem um `Dir` com os valores correspondentes e, se não for uma inversão, atualizam `nextDir`.

### Mouse — câmera orbital

O botão esquerdo do mouse captura o início do arrasto em `mouseButton`. Durante o movimento com o botão pressionado (`mouseMotion`), o deslocamento do cursor é convertido em rotação da câmera:

```cpp
camAngleH += dx * 0.5f;  // rotação horizontal
camAngleV -= dy * 0.5f;  // rotação vertical (invertida: arrasto para baixo = câmera sobe)
```

O ângulo vertical é limitado entre 5° e 89° para evitar que a câmera passe pelo plano do grid ou vire de cabeça para baixo. Só funciona no modo de câmera de 3ª pessoa (`camMode == 0`).

---

## Câmeras

Há três modos de câmera, alternados pela tecla C (`camMode` 0→1→2→0):

| Modo | Descrição |
|---|---|
| **3ª pessoa** (0) | Câmera orbital com `camDist = 28`, controlada pelo mouse. Usa `gluLookAt` a partir de uma posição esférica calculada com `camAngleH` e `camAngleV`. |
| **1ª pessoa** (1) | Câmera posicionada na cabeça da cobra, olhando na direção de movimento (`curDir`). |
| **Topo** (2) | Câmera acima do grid, olhando diretamente para baixo (`gluLookAt` com up = (0,0,-1)). |

---

## Portais (`checkPortal`)

Quando a parede central surge (a cada 5 maçãs), dois portais são gerados — um de cada lado da parede. A função `checkPortal` é chamada logo após calcular a nova posição da cabeça:

1. Verifica se `head` coincide com `PORTAL_A` ou `PORTAL_B`.
2. Redireciona a cabeça para a célula imediatamente após o portal de saída, na direção atual de movimento.
3. Se essa célula estiver bloqueada (parede ou fora do grid), a cobra sai no próprio portal.
4. Registra um cooldown de **3 passos** para evitar que a cobra entre e saia do portal em sequência imediata.
