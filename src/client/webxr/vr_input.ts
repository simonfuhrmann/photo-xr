import * as THREE from 'three';
import { XRControllerModelFactory } from 'three/examples/jsm/webxr/XRControllerModelFactory.js';

const controllerModelFactory = new XRControllerModelFactory();

export class VRInput extends EventTarget {
  private renderer: THREE.WebGLRenderer;
  private scene: THREE.Scene;
  private buttons: Map<XRInputSource, boolean[]> = new Map();

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
        this.onButton(source, index, button.pressed);
      });
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

  private onButton(source: XRInputSource, button: number, pressed: boolean) {
    const buttons = this.buttons.get(source) || [];
    if ((buttons[button] ?? false) === pressed) return; // No change.
    buttons[button] = pressed;
    this.buttons.set(source, buttons);

    const detail = { source, button, pressed };
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
