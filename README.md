### Archivos clave

- **Mapa / Estado**
  - `MapGridSubsystem.h/.cpp` – Representa el grid lógico (obstáculos, passability, spawns, base, waves).
  - `MapGenerator.h/.cpp` – Construye la **vista** por instanced meshes según el grid.
  - `MapConfigAsset.h/.cpp` – Asset con configuración y datos de mapa (legend, filas, waves).

- **Juego base**
  - `BattleGameMode.h/.cpp` – Reglas (vidas, victoria/derrota, respawns).
  - `BattleGameInstance.h/.cpp` – Glue de juego (si aplica).

- **Jugador / Enemigos / Combate**
  - `TankPawn.h/.cpp` – Tanque del jugador (movimiento cardinal, disparo).
  - `EnemyPawn.h/.cpp` – Enemigo genérico (usa componentes/policies).
  - `Projectile.h/.cpp` – Proyectiles, colisiones deterministas con grid (rompen ladrillos).
  - `BattleBase.h/.cpp` – “Águila”/Base; su destrucción provoca derrota.

- **Oleadas / Spawns**
  - `EnemySpawner.h/.cpp` – Orquesta waves, límites de vivos, **políticas** de objetivo y spawn point.
  - `EnemySpawnPointPolicy_*.h/.cpp` – Políticas de selección de punto de spawn.

- **IA por Políticas (Goals/Movement)**
  - `EnemyGoalPolicy*.h/.cpp` – A qué objetivo prioriza la IA (base/jugador), con reglas:
    - `RandomFixed`, `AdvantageBias` (histéresis), `WeightedDynamic`.
  - `EnemyMovementComponent.h/.cpp` – Componente que decide movimiento **vía**:
    - `EnemyMovePolicy.h/.cpp` (base)
    - `EnemyMovePolicy_GridAxisLock.h/.cpp` (lock de eje + “stop & shoot” ante ladrillo frontal).

---

## 🧠 Arquitectura de IA (políticas enchufables)

### 1) Objetivo (GoalPolicy) – en **EnemySpawner**
- Selecciona/actualiza la **meta** de cada enemigo: `HuntBase` o `HuntPlayer`.
- Políticas incluidas:
  - `RandomFixed` – Asignación aleatoria al spawn, **no** cambia.
  - `AdvantageBias` – Cambia **globalmente** según vivos (umbrales con histéresis).
  - `WeightedDynamic` – Reevaluación periódica, con sesgo y jitter; per-enemigo o global.
- Uso: en el `EnemySpawner` del nivel, selecciona **GoalPolicyClass** y luego edita la instancia inline.

### 2) Movimiento (MovePolicy) – vía **EnemyMovementComponent** del EnemyPawn
- El `EnemyMovementComponent` construye un **contexto de movimiento** (pos, facing, target, consultas al grid) y delega en la **MovePolicy**.
- Política incluida:
  - `EnemyMovePolicy_GridAxisLock`:
    - Evita “bamboleo” diagonal (bloqueo de eje + deadband).
    - Si hay **ladrillo frontal**, **se detiene y dispara** (no gira a la ortogonal).
    - Look-ahead para evitar girar contra paredes.
- Uso: en el componente **EnemyMovement** del `EnemyPawn`, selecciona **MovePolicyClass** y ajusta tunings (`MinLockTime`, `AlignEpsilonFactor`, etc.).

### 3) Punto de aparición (SpawnPointPolicy) – en **EnemySpawner**
- Decide el **orden de prueba** de puntos de spawn (el Spawner evita encimarse y reintenta).
- Políticas incluidas:
  - `RandomAny` – Aleatorio entre todos los puntos válidos (unión de símbolos, `.` siempre vacío).
  - `FarFromPlayer` – Prioriza puntos lejanos al jugador (con umbral configurable).

> Todas las políticas usan el patrón **Class + Instance (EditInlineNew)** para ediciones cómodas en el editor y evitar recursión del Property Editor.

---

## 🗺️ Mapas y Waves

- Los datos de mapa viven en un **`UMapConfigAsset`**:
  - `width`, `height`, `tileSize`
  - `rows[]` – el layout del grid (caracteres por tile)
  - `legend{}` – símbolos → tipos de tile (p. ej. `"."` vacío, `"B"` ladrillo, `"S"` acero, etc.)
  - `waves[]` – tiempo, tipo de enemigo, cantidad, etc.

> **Spawn points**: el spawner usa la **unión** de todos los símbolos marcados como spawn (ignorando `"."`).

---

## 🎮 Comportamiento in-game (resumen)

- Movimiento y disparo **cardinal** (N/E/S/O).
- Los proyectiles destruyen **ladrillo**, no **acero**.
- Enemigos:
  - Objetivo (base/jugador) según **GoalPolicy**.
  - Movimiento y “stop & shoot” según **MovePolicy**.
  - Spawneo sin encimarse, respetando `MaxAlive`.

---

## 🧪 Debug & Telemetría

- HUD simple desde `EnemySpawner`:
  - Consola del juego:
    ```
    bc_ai_debug 1
    ```
  - Muestra: `Alive`, `Spawned/Planned`, `HB/HP` y policy activa.
  - Desactivar: `bc_ai_debug 0`.

---

## ⚙️ Parámetros útiles

- **EnemySpawner**
  - `MaxAlive`, `GoalPolicyClass` (+ props específicas), `SpawnPointPolicyClass` (+ props).

- **EnemyMovementComponent (en EnemyPawn)**
  - `MovePolicyClass`
  - `MinLockTime`, `AlignEpsilonFactor`, `TieDeadbandFactor`, `LookAheadTiles`
  - `bPreferShootWhenFrontBrick`, `FrontCheckTiles`

---

## 🧩 Extender el proyecto

- **Nueva GoalPolicy**: deriva de `UEnemyGoalPolicy` y sobreescribe `DecideGoalOnSpawn` / `TickPolicy`. Selección en el spawner.
- **Nueva MovePolicy**: deriva de `UEnemyMovePolicy` y sobreescribe `ComputeMove`. Conecta en `EnemyMovementComponent`.
- **Nueva SpawnPointPolicy**: deriva de `UEnemySpawnPointPolicy` y sobreescribe `BuildCandidateOrder`.

> Ideas: `Grid_AStarPolicy` (path corto por tiles), `NearBase/EdgesOnly` para spawn, `CombatPolicy` separada si quieres IA de disparo más sofisticada.

---

## 🛠️ Notas de diseño

- El **grid real** vive en `MapGridSubsystem`; la **vista** la genera `MapGenerator`.
- Colisiones deterministas (romper ladrillos) se hacen contra el **grid** (además de física).
- Se prefiere composición y políticas **instanciables** para minimizar acoplamientos.

---

## 🧭 Roadmap sugerido

- HUD de editor con más métricas (disparos limpios vs. a ladrillo).
- `Grid_AStarPolicy` opcional para enemigos “power”.
- Override por **Wave** (elegir Goal/Move/SpawnPolicy por oleada).
- QA automática (sim runner) con export de métricas a CSV.

---

## 📜 Licencia

(Sin licencia definida aún; añade la que prefieras: MIT/BSD/Apache-2.0.)

---

## 🤝 Créditos

- Código y diseño: (Fimbul).
- “Battle City” es marca/obra de Namco (1985). Proyecto no comercial y con fines educativos.
"""
