import * as THREE from 'three';
import { XRControllerModelFactory } from 'three/examples/jsm/webxr/XRControllerModelFactory.js';

const controllerModelFactory = new XRControllerModelFactory();

export enum StickDir {
  CENTER,
  LEFT,
  RIGHT,
  UP,
  DOWN
}

export type ButtonEvent = { gamepad: Gamepad, button: number, pressed: boolean };
export type DirectionEvent = { gamepad: Gamepad, direction: StickDir };

export class VRInput extends EventTarget {
  private renderer: THREE.WebGLRenderer;
  private scene: THREE.Scene;
  private buttons: Map<Gamepad, boolean[]> = new Map();
  private stickDir: Map<Gamepad, StickDir> = new Map();

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

  private handleAxesXY(gamepad: Gamepad, axisX: number, axisY: number) {
    const ACTIVATE = 0.6;   // Minimum magnitude to trigger.
    const RELEASE = 0.3;   // Must return to this value to reset.
    const absX = Math.abs(axisX);
    const absY = Math.abs(axisY);

    if (absX < RELEASE && absY < RELEASE) {
      this.onDirection(gamepad, StickDir.CENTER);
    } else if (absX > absY && absX > ACTIVATE) {
      this.onDirection(gamepad, axisX > 0 ? StickDir.RIGHT : StickDir.LEFT);
      return;
    } else if (absY > absX && absY > ACTIVATE) {
      this.onDirection(gamepad, axisY > 0 ? StickDir.UP : StickDir.DOWN);
      return;
    }
  }

  private onDirection(gamepad: Gamepad, dir: StickDir) {
    if (this.stickDir.get(gamepad) === dir) return; // No change.
    this.stickDir.set(gamepad, dir);
    const detail = { gamepad, direction: dir };
    this.dispatchEvent(new CustomEvent('direction', { detail }));
  }

  private onButton(gamepad: Gamepad, button: number, pressed: boolean) {
    const buttons = this.buttons.get(gamepad) || [];
    if ((buttons[button] ?? false) === pressed) return; // No change.
    buttons[button] = pressed;
    this.buttons.set(gamepad, buttons);

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
