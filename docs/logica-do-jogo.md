# Lógica do Jogo

## Estado global

O jogo mantém todo o seu estado em variáveis globais estáticas:

| Variável | Tipo | Descrição |
|---|---|---|
| `snake` | `vector<Cell>` | Segmentos da cobra; índice 0 = cabeça |
| `curDir` / `nextDir` | `Dir` | Direção atual e direção pendente do próximo passo |
| `gridState[GRID][GRID]` | `int` | Estado de cada célula: `CT_EMPTY` (0) ou `CT_WALL` (1) |
| `food` | `Cell` | Posição da maçã atual |
| `wallsActive` | `bool` | Se a parede central foi gerada |
| `portalsActive` | `bool` | Se os portais estão ativos |
| `portalCooldown` | `int` | Passos restantes de imunidade ao portal |
| `score` | `int` | Pontuação acumulada |
| `applesEaten` | `int` | Total de maçãs comidas (controla quando gerar parede) |
| `gameOver` | `bool` | Flag de fim de jogo |
| `stepMs` | `int` | Intervalo do timer em milissegundos (velocidade atual) |

---

## Inicialização (`initGame`)

Ao iniciar ou reiniciar a partida (tecla R), `initGame` executa:

1. Zera `gridState` com `memset` — remove todas as paredes.
2. Reseta flags: `wallsActive = false`, `portalsActive = false`, `portalCooldown = 0`.
3. Constrói a cobra inicial com **3 segmentos** centrados no grid:
   - Cabeça em `(GRID/2, GRID/2)`
   - Dois segmentos de corpo à esquerda
4. Define `curDir = nextDir = {1, 0}` (movimento inicial para a direita).
5. Reseta `score`, `applesEaten`, `gameOver` e `stepMs = STEP_MS_INIT`.
6. Chama `spawnFood()` para posicionar a primeira maçã.

---

## Spawn de comida (`spawnFood`)

A maçã é colocada em uma célula aleatória válida — não pode estar:

- Ocupada por algum segmento da cobra (`cellInSnake`)
- Em uma célula de parede (`gridState == CT_WALL`)
- Em uma das células de portal (quando ativos)

Um laço tenta até `GRID × GRID × 4` vezes antes de desistir (evita loop infinito em grids muito cheios). Na prática, com GRID = 30, há espaço suficiente mesmo com a cobra crescida.

---

## Passo do jogo (`step`)

A cada intervalo de `stepMs` milissegundos:

1. `curDir ← nextDir`
2. Calcula nova posição da cabeça
3. `checkPortal` — teleporta se necessário
4. Decrementa `portalCooldown`
5. **Colisão com borda** → `gameOver = true` + som de game over
6. **Colisão com parede** → `gameOver = true` + som de game over
7. **Auto-colisão** (exceto último segmento, que será removido) → `gameOver = true` + som de game over
8. Insere nova cabeça em `snake[0]`
9. **Se comeu a maçã:**
   - `score += 10`
   - `applesEaten++`
   - Toca som de comer
   - Reduz `stepMs` (acelera o jogo)
   - A cada `APPLES_PER_WALL` (5) maçãs, chama `spawnWall()`
   - Chama `spawnFood()` para nova maçã
   - **Não remove a cauda** → cobra cresce
10. **Se não comeu:** remove `snake.back()` → cobra mantém o tamanho

---

## Velocidade progressiva

A velocidade do jogo aumenta automaticamente conforme o jogador come maçãs:

```
stepMs inicial : 150 ms  (~6,7 passos/s)
redução        :   5 ms por maçã
stepMs mínimo  :  60 ms  (~16,7 passos/s)
```

Para atingir a velocidade máxima são necessárias 18 maçãs (`(150 - 60) / 5 = 18`).

---

## Parede central e portais

### Geração da parede (`spawnWall`)

A cada múltiplo de `APPLES_PER_WALL` maçãs comidas, `spawnWall` é chamada uma vez:

1. Preenche toda a coluna `WALL_COLUMN = GRID / 2` com `CT_WALL` em `gridState`.
2. Define `wallsActive = true`.
3. Chama `spawnPortals()` e `portalsActive = true`.
4. Toca o som de parede.

A parede tem altura visual de 2 células (`CELL * 2.0`) para aparecer como um obstáculo sólido acima do chão.

### Geração dos portais (`spawnPortals`)

Dois portais são gerados, um de cada lado da parede:

- **Portal A** — em `x < WALL_COLUMN` (lado esquerdo), posição Z aleatória
- **Portal B** — em `x > WALL_COLUMN` (lado direito), posição Z aleatória

Ambos evitam células da cobra, paredes e a posição da comida atual.

### Teleporte (`checkPortal`)

Quando a cabeça entra em um portal, é reposicionada na célula imediatamente após o portal de saída na direção atual. O cooldown de 3 passos impede que a cobra entre imediatamente no outro portal após sair.

---

## Colisões e game over

Três condições encerram a partida:

| Condição | Verificação |
|---|---|
| **Saída do grid** | `!cellInGrid(head.x, head.z)` |
| **Colisão com parede** | `gridState[head.x][head.z] == CT_WALL` |
| **Auto-colisão** | `head == snake[i]` para qualquer `i` de 0 até `size-2` |

Em qualquer um dos casos: `gameOver = true`, o som de game over é tocado e o timer para de se re-agendar. O jogo continua renderizando normalmente, mas com a sobreposição da tela de fim.

---

## HUD (display 2D)

O HUD é desenhado sobre a cena 3D usando projeção ortográfica (`gluOrtho2D`), desativando depth test e iluminação temporariamente.

Elementos exibidos:

- **Linha superior**: pontuação atual, total de maçãs comidas e nome do modo de câmera ativo
- **Aviso de parede**: quando ainda não surgiu, mostra quantas maçãs faltam para a próxima parede
- **Linha inferior**: lista de controles
- **Tela de game over**: painel semitransparente (`GL_BLEND`) com "GAME OVER!", pontuação final e instrução para reiniciar

O texto é renderizado com `glutBitmapCharacter` (fonte `GLUT_BITMAP_HELVETICA_18`, `12` e `GLUT_BITMAP_TIMES_ROMAN_24`), chamado caractere a caractere a partir de `glRasterPos2f`.
