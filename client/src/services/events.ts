// eventBus.ts
import { EventEmitter } from 'eventemitter3';

// Create and export a singleton event bus instance
const eventBus = new EventEmitter();
export default eventBus;
