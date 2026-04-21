# Design Patterns Used

## Structural Patterns

### Adapter Pattern
- **Location**: `Core/Adapters/`
- **Purpose**: Wraps mpg123/LAME libraries behind `AudioFileAdapter` interface
- **Classes**: `AudioFileAdapter` (target), `Mp3Adapter` (adapter)
- **Benefit**: Swap audio backends without changing client code

### Composite Pattern
- **Location**: `Core/Logging/`
- **Purpose**: Treat individual loggers and logger groups uniformly
- **Classes**: `ILogger` (component), `CompositeLogger` (composite), `FileLogger`/`ConsoleLogger` (leaves)
- **Benefit**: Add file logging, console logging, or both with same interface

## Behavioral Patterns

### Strategy Pattern
- **Location**: `Core/Effects/`
- **Purpose**: Interchangeable audio processing algorithms
- **Classes**: `IEffect` (strategy), `Reverb`/`Speed`/`Volume`/`Normalize` (concrete strategies)
- **Benefit**: Add new effects without modifying existing code

### Command Pattern
- **Location**: `Core/Commands/`
- **Purpose**: Encapsulate operations for undo/redo support
- **Classes**: `ICommand` (command), `ApplyEffectCommand`/`EffectStateCommand` (concrete), `CommandHistory` (invoker)
- **Benefit**: Full undo/redo history, command logging

### Observer Pattern
- **Location**: Throughout GUI layer
- **Purpose**: Decouple UI components from audio engine state
- **Implementation**: Qt signals/slots (`AudioEngine` → `TransportBar`, `WaveformWidget`, `CaptionPanel`)
- **Benefit**: UI updates automatically when playback state changes

## Creational Patterns

### Factory Pattern
- **Location**: `Core/EffectFactory.h`
- **Purpose**: Create effect instances by name with dependency injection
- **Implementation**: Registry-based factory with lambda creators
- **Benefit**: Add new effects by registering, no switch statements

## Architectural Decisions

### Layered Architecture
```
GUI/          → Presentation layer (Qt widgets)
Core/         → Business logic (effects, commands, audio processing)
  Adapters/   → External library integration
  Commands/   → Undo/redo infrastructure
  Effects/    → Audio processing strategies
  Logging/    → Cross-cutting concern
  Services/   → Utilities (caption parsing)
```

### Dependency Injection
All major classes receive `ILogger` via constructor, enabling:
- Unit testing with mock loggers
- Runtime logger configuration
- Consistent logging across components
