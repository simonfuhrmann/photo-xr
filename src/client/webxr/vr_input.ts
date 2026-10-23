import * as THREE from 'three';
import { XRControllerModelFactory } from 'three/examples/jsm/webxr/XRControllerModelFactory.js';

const controllerModelFactory = new XRControllerModelFactory();

export enum StickDir {
  CENTER,
  LEFT,
  RIGHT,
  UP,
  DOWN,
}

export interface ButtonEvent {
  gamepad: Gamepad;
  button: number;
  pressed: boolean;
}

export interface DirectionEvent {
  gamepad: Gamepad;
  direction: StickDir;
}

export interface GamepadState {
  // The pressed state of each button by button index. Note that elements can
  // be undefined and should be treated as "not pressed". To test the state of
  // a button, use `buttons[i] ?? false`.
  buttons: boolean[];

  // The direction of the thumb stick. The controller is assumed to have one
  // thumbstick, and the axis with max magnitude takes precedence. A generous
  // dead zone is applied.
  stickDir: StickDir;

  // The raw (X, Y) value of the thumb stick in [-1, 1] x [-1, 1]. The magnitude
  // of (X, Y) is ~0 at rest, and ~1 when fully pressed in a direction.
  axisX: number;
  axisY: number;
}

export class VRInput extends EventTarget {
  private renderer: THREE.WebGLRenderer;
  private scene: THREE.Scene;
  private state: Map<Gamepad, GamepadState> = new Map();

  constructor(renderer: THREE.WebGLRenderer, scene: THREE.Scene) {
    super();
    this.renderer = renderer;
    this.scene = scene;

    // Register select and squeeze events for each controller.
    for (let i = 0; i < 2; i++) {
      const controller = this.renderer.xr.getController(i);
      controller.addEventListener('select',
        this.onSelect.bind(this, controller));
      controller.addEventListener('squeeze',
        this.onSqueeze.bind(this, controller));
    }
  }

  // Create two controllers, and add controller models and their rays.
  // This MUST ONLY be called once.
  addControllerModels() {
    for (let i = 0; i < 2; i++) {
      const controller = this.renderer.xr.getController(i);
      controller.add(getRayLine());
      this.scene.add(controller);

      const grip = this.renderer.xr.getControllerGrip(i);
      grip.add(controllerModelFactory.createControllerModel(grip));
      this.scene.add(grip);
    }

    // The controllers need to be lit. Add light to the scene.
    this.scene.add(getHemisphereLight());
    this.scene.add(getDirectionalLight());
  }

  pollInputs() {
    const session = this.renderer.xr.getSession();
    if (!session) return;
    session.inputSources.forEach((source) => {
      const gamepad = source.gamepad;
      if (!gamepad) return;
      gamepad.buttons.forEach((button, index) => {
        this.onButton(gamepad, index, button.pressed);
      });
      this.handleAxes(gamepad);
    });
  }

  private onSelect(controller: THREE.Object3D, event: unknown) {
    const detail = { controller, event };
    this.dispatchEvent(new CustomEvent('select', { detail }));
  }

  private onSqueeze(controller: THREE.Object3D, event: unknown) {
    const detail = { controller, event };
    this.dispatchEvent(new CustomEvent('squeeze', { detail }));
  }

  // Select the x/y axes with the largest magnitude.
  private handleAxes(gamepad: Gamepad) {
    const x0 = gamepad.axes[0] ?? 0.0;
    const y0 = gamepad.axes[1] ?? 0.0;
    const x1 = gamepad.axes[2] ?? 0.0;
    const y1 = gamepad.axes[3] ?? 0.0;
    const mag0 = x0 * x0 + y0 * y0;
    const mag1 = x1 * x1 + y1 * y1;
    const axisX = mag0 > mag1 ? x0 : x1;
    const axisY = mag0 > mag1 ? y0 : y1;
    this.handleAxesXY(gamepad, axisX, axisY);
  }

  // The (axisX, axisY) report values in [-1, 1] x [-1, 1], where (-1, 0) is
  // the stick pushed to the left, and (0, -1) is the stick pushed to the top.
  // The magnitude of (axisX, axisY) is 0 at rest, and ~1 when fully pushed.
  private handleAxesXY(gamepad: Gamepad, axisX: number, axisY: number) {
    const stickDir = getStickDirection(axisX, axisY);
    const state = this.state.get(gamepad) ?? getInitGamepadState();
    const stickDirChange = stickDir !== state.stickDir;

    state.axisX = axisX;
    state.axisY = axisY;
    state.stickDir = stickDir ?? state.stickDir;
    this.state.set(gamepad, state);

    if (stickDir && stickDirChange) {
      const detail = { gamepad, direction: stickDir };
      this.dispatchEvent(new CustomEvent('direction', { detail }));
    }
  }

  private onButton(gamepad: Gamepad, button: number, pressed: boolean) {
    const state = this.state.get(gamepad) ?? getInitGamepadState();
    if ((state.buttons[button] ?? false) === pressed) return; // No change.
    state.buttons[button] = pressed;
    this.state.set(gamepad, state);

    const detail = { gamepad, button, pressed };
    this.dispatchEvent(new CustomEvent('button', { detail }));
  }
}

function getRayLine() {
  const geometry = new THREE.BufferGeometry().setFromPoints([
    new THREE.Vector3(0, 0, 0),
    new THREE.Vector3(0, 0, -100)
  ]);
  const line = new THREE.Line(geometry);
  line.position.y = -0.005;
  line.scale.z = 5;
  return line;
}

function getHemisphereLight() {
  return new THREE.HemisphereLight(
    /*skyColor=*/0xffffff,
    /*groundColor=*/0x444444,
    /*intensity=*/1.0
  );
}

function getDirectionalLight() {
  const dirLight = new THREE.DirectionalLight(0xffffff, 0.5);
  dirLight.position.set(1, 1, 1);
  return dirLight;
}

function getInitGamepadState(): GamepadState {
  return {
    axisX: 0,
    axisY: 0,
    buttons: [],
    stickDir: StickDir.CENTER,
  };
}

// Returns the stick direction for coordinates with a clear direction, but
// returns `undefined` for many cases, e.g., to implement hysteresis. When
// `undefined` is returned, the previous state must be assumed.
function getStickDirection(axisX: number, axisY: number): StickDir | undefined {
  const ACTIVATE = 0.6;  // Minimum magnitude to trigger.
  const RELEASE = 0.3;   // Must return to this value to reset.
  const absX = Math.abs(axisX);
  const absY = Math.abs(axisY);

  if (absX < RELEASE && absY < RELEASE) {
    return StickDir.CENTER;
  } else if (absX > absY && absX > ACTIVATE) {
    return axisX > 0 ? StickDir.RIGHT : StickDir.LEFT;
  } else if (absY > absX && absY > ACTIVATE) {
    return axisY > 0 ? StickDir.DOWN : StickDir.UP;
  }
  return undefined;
}
