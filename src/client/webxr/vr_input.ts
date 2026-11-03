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

// https://www.w3.org/TR/webxr-gamepads-module-1/#example-mappings
export enum ButtonId {
  TRIGGER = 0,
  SQUEEZE = 1,
  TOUCHPAD = 2,
  THUMBSTICK = 3,
  BUTTON_A = 4,
  BUTTON_B = 5,
}

export interface ButtonEvent {
  gamepad: Gamepad;
  index: number;
  pressed: boolean;
}

export interface DirectionEvent {
  gamepad: Gamepad;
  direction: StickDir;
}

export interface GamepadState {
  // A boolean pressed state, and value state in [0,1] of each button by button
  // index. Usually, pressed is true if value > 0. Note that array elements can
  // be undefined and should be treated as "not pressed" or 0, respectively.
  // To get the pressed state of a button, use `buttons[i] ?? false`, or to get
  // the value state of a button, use `buttonValues[i] ?? 0.0`.
  buttons: boolean[];
  buttonValues: number[];

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
  private group = new THREE.Group();
  private state: Map<Gamepad, GamepadState> = new Map();

  constructor(renderer: THREE.WebGLRenderer, scene: THREE.Scene) {
    super();
    this.renderer = renderer;
    this.scene = scene;
    this.scene.add(this.group);

    // Register select and squeeze events for each controller.
    for (let i = 0; i < 2; i++) {
      const controller = this.renderer.xr.getController(i);
      controller.addEventListener('select',
        this.onSelect.bind(this, controller));
      controller.addEventListener('squeeze',
        this.onSqueeze.bind(this, controller));
    }
  }

  getState(): Map<Gamepad, GamepadState> {
    return this.state;
  }

  // Create two controllers, and add controller models and their rays.
  // This MUST ONLY be called once.
  addControllerModels() {
    for (let i = 0; i < 2; i++) {
      const controller = this.renderer.xr.getController(i);
      controller.add(getRayLine());
      this.group.add(controller);

      const grip = this.renderer.xr.getControllerGrip(i);
      grip.add(controllerModelFactory.createControllerModel(grip));
      this.group.add(grip);
    }

    // The controllers need to be lit. Add light to the scene.
    this.group.add(getHemisphereLight());
    this.group.add(getDirectionalLight());
  }

  // The spec considers two types of controllers see
  // https://www.w3.org/TR/webxr-gamepads-module-1/#example-mappings
  //
  // 1. Controllers with a touchpad, with axes[0] and axes[1], and the touchpad
  //    button is buttons[2].
  // 2. Controllers with a thumbstick, with axes[2] and axes[3], and the
  //    thumbsick button is buttons[3].
  //
  // Controllers always have a trigger button (buttons[0]), can have a
  // squeeze (buttons[1]), can have an A-button (buttons[4]) and can have a
  // B-button (buttons[5]).
  //
  // Each button state also has a value in [0,1] for intermediate presses on
  // analog buttons (usually the trigger and squeeze buttons). Binary buttons
  // such as A and B buttons also report a value, but just either 0 or 1. The
  // pressed state of a button is true if value > 0.
  pollInputs() {
    const session = this.renderer.xr.getSession();
    if (!session) return;
    session.inputSources.forEach((source) => {
      if (!source.gamepad) return;
      const state = this.state.get(source.gamepad) ?? getInitGamepadState();
      this.handleButtons(source.gamepad, state);
      this.handleAxes(source.gamepad, state);
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

  private handleButtons(gamepad: Gamepad, state: GamepadState) {
    gamepad.buttons.forEach((button, index) => {
      const pressed = button.pressed;
      const value = button.value;

      const pressedChange = (state.buttons[index] ?? false) !== pressed;
      state.buttons[index] = pressed;
      state.buttonValues[index] = value;
      this.state.set(gamepad, state);

      // Emit 'button' event on pressed state change.
      if (pressedChange) {
        const detail = { gamepad, index, pressed };
        this.dispatchEvent(new CustomEvent('button', { detail }));
      }
    });
  }

  // The (axisX, axisY) report values in [-1, 1] x [-1, 1], where (-1, 0) is
  // the stick pushed to the left, and (0, -1) is the stick pushed to the top.
  // The magnitude of (axisX, axisY) is 0 at rest, and ~1 when fully pushed.
  private handleAxes(gamepad: Gamepad, state: GamepadState) {
    // Select the x/y axes with the largest magnitude.
    const x0 = gamepad.axes[0] ?? 0.0;
    const y0 = gamepad.axes[1] ?? 0.0;
    const x1 = gamepad.axes[2] ?? 0.0;
    const y1 = gamepad.axes[3] ?? 0.0;
    const mag0 = x0 * x0 + y0 * y0;
    const mag1 = x1 * x1 + y1 * y1;
    const axisX = mag0 > mag1 ? x0 : x1;
    const axisY = mag0 > mag1 ? y0 : y1;

    // Update the state.
    const stickDir = getStickDirection(axisX, axisY);
    const stickDirChange = stickDir !== state.stickDir;
    state.axisX = axisX;
    state.axisY = axisY;
    state.stickDir = stickDir ?? state.stickDir;
    this.state.set(gamepad, state);

    // Emit 'direction' event on stick direction state change.
    if (stickDir && stickDirChange) {
      const detail = { gamepad, direction: stickDir };
      this.dispatchEvent(new CustomEvent('direction', { detail }));
    }
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
    buttonValues: [],
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
