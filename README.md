# Battle City 3D (Unreal Engine 5)

Este proyecto es una reimplementación moderna y en 3D de las mecánicas clásicas de *Battle City* (Namco, 1985) desarrollada en **Unreal Engine 5.6+** utilizando C++.

El proyecto se destaca por una arquitectura modular basada en **Componentes** y **Políticas (Strategy Pattern)**, separando claramente la lógica de juego (Grid), la vista (Instanced Meshes) y la toma de decisiones de la IA.

---

## 📂 Estructura del Proyecto y Archivos Clave

### 1. Core & Reglas de Juego
* **`BattleGameMode`**: Orquesta el ciclo de vida de la partida. Gestiona el *spawneo* de la base (Águila), vincula el `EnemySpawner`, controla las condiciones de Victoria/Derrota y el respawn del jugador.
* **`BattlePlayerController`**: Configura el sistema de **Enhanced Input** (`IMC_Tank`) y gestiona la posesión del Pawn.
* **`BattleGameInstance`**: Subsistema persistente del juego.

### 2. Entidades (Tanques y Combate)
* **`Common/BattleTankPawn`** (Clase Base): Centraliza la física de movimiento compartida. Implementa el sistema de **colisión determinista** usando "bigotes" (raycasts) contra el Grid y el **snap al subgrid** para movimiento cardinal fluido.
* **`Player/TankPawn`**: Hereda de la base. Gestiona el input del jugador y el disparo.
* **`Enemies/EnemyPawn`**: Hereda de la base. Posee el `EnemyMovementComponent` (el "cerebro") y define stats (HP, Velocidad) según el tipo (`Basic`, `Fast`, `Power`, `Armored`).
* **`Projectiles/Projectile`**: Implementa una **detección volumétrica** contra el Grid para destruir ladrillos de forma precisa y colisiones por barrido (`Sweep`) contra actores dinámicos.
* **`BattleBases/BattleBase`**: La base a defender. Su destrucción detona el *Game Over*.

### 3. Mapa y Sistema de Grid
* **`MapGridSubsystem`**: Representa el estado lógico del mundo. Gestiona la matriz de terrenos (`Ice`, `Water`, `Forest`) y obstáculos (`Brick`, `Steel`), así como su salud.
* **`MapGenerator`**: Se encarga exclusivamente de la representación visual utilizando **Instanced Static Meshes (ISM)** para optimizar el rendimiento.
* **`MapConfigAsset`**: DataAsset que almacena la configuración del nivel (dimensiones, layout, oleadas).
* **`MapConfigImporter` (Plugin)**: Plugin de editor que permite importar archivos `.json` directamente como assets de mapa.

---

## 🧠 Arquitectura de IA (Sistema de Políticas)

La IA utiliza un diseño desacoplado donde el comportamiento se define mediante la composición de pequeñas políticas lógicas.

### 1. Movimiento (`EnemyMovementComponent`)
El componente `EnemyMovementComponent` solicita inputs de movimiento al Pawn basándose en una **Move Policy** intercambiable:

* **`GridAxisLock`**: Movimiento básico cardinal. Incluye lógica "Stop & Shoot" si detecta un ladrillo bloqueando el camino directo.
* **`PathFollow`**: Utiliza el subsistema `GridPathManager` (A*) para calcular y seguir rutas complejas hacia el objetivo.
* **`WanderFar`**: Deambula aleatoriamente si el objetivo está muy lejos o el camino está bloqueado.
* **`ShootWhenBlocking`**: Sugiere disparar si hay un obstáculo destructible inmediatamente enfrente.
* **`Composite`**: Permite combinar múltiples políticas (ej. *PathFollow* + *ShootWhenBlocking*) ejecutándolas secuencialmente y fusionando sus decisiones.

### 2. Objetivos (`GoalPolicy`) - En `EnemySpawner`
Define qué prioriza el enemigo: ¿Atacar la Base o cazar al Jugador?

* **`RandomFixed`**: Asigna un objetivo fijo al nacer basado en una probabilidad.
* **`AdvantageBias`**: Cambia el objetivo de todos los enemigos dinámicamente según cuántos aliados queden vivos (comportamiento de manada).
* **`WeightedDynamic`**: Reevalúa periódicamente el objetivo con probabilidades ponderadas.

### 3. Aparición (`SpawnPointPolicy`) - En `EnemySpawner`
Controla la selección del punto de nacimiento para evitar colisiones y mejorar el flujo.

* **`RandomAny`**: Elige aleatoriamente entre todos los puntos válidos definidos en el mapa.
* **`FarFromPlayer`**: Prioriza los puntos de aparición más lejanos a la posición actual del jugador.

---

## 🗺️ Formato de Mapa (JSON)

Los niveles se definen en archivos JSON ubicados en la carpeta del proyecto. El plugin `MapConfigImporter` los procesa automáticamente.

**Ejemplo de estructura (`Prototype.json`):**

```json
{
  "width": 26,
  "height": 26,
  "tileSize": 100.0,
  "rows": [
    "..SSSS..FFFF......BBBBBBBB",
    "........P.........BBBBV.BB",
    "....WWWW...........BBBBBBB"
  ],
  "legend": {
    ".": {"terrain":"Ground"},
    "I": {"terrain":"Ice"},
    "W": {"terrain":"Water"},
    "F": {"terrain":"Forest"},
    "B": {"obstacle":"Brick"},
    "S": {"obstacle":"Steel"},
    "P": {"playerStart": true},
    "A": {"enemySpawn": "Basic"},
    "T": {"enemySpawn": "Armored"}
  },
  "waves": [
    { "time": 3.0,  "type":"Basic",  "spawn":"A" },
    { "time": 35.0, "type":"Armored","spawn":"T" }
  ]
}

---
## 🛠️ Herramientas de Depuración (Console Variables)

Abre la consola en juego (`~`) para utilizar estas herramientas de visualización:

| Comando | Valores | Descripción |
| :--- | :---: | :--- |
| `bc.ai.debug` | `0` / `1` | Muestra en pantalla el estado de la IA: Cantidad de vivos, oleadas pendientes y política activa. |
| `bc.map.debug` | `0` / `1` | Dibuja las líneas del Grid lógico y el Subgrid sobre el terreno. |
| `bc.collision.debug` | `0` / `1` | Visualiza los "bigotes" de colisión de los tanques (Verde = Libre, Rojo = Bloqueado) y los bounds de las bases. |

---

## 🚀 Guía de Extensión

### Añadir un Nuevo Enemigo
1. Crea un Blueprint hijo de `BP_EnemyPawn`.
2. Configura sus estadísticas (`HitPoints`, `MoveSpeed`) en el panel de detalles.
3. En el componente `EnemyMovement`, asigna una **Move Policy Class** (ej. `EnemyMovePolicy_Composite`).
4. Registra el nuevo enemigo en el `EnemySpawner` del nivel (sección "Spawn Clases").

### Crear una Nueva Política de Movimiento
1. Crea una clase C++ que herede de `UEnemyMovePolicy`.
2. Sobrescribe el método `ComputeMove(const FMoveContext& Ctx, FMoveDecision& Out)`.
3. Utiliza el contexto (`Ctx`) para consultar el Grid sin acceder directamente a los actores.
4. Compila y asígnala en el editor.

---

## 📜 Licencia y Créditos

**Licencia MIT**
Copyright (c) 2025 FimbulSW

Este proyecto es de carácter educativo. "Battle City" es una marca original de Namco.
