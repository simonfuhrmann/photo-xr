import { MediaInterface } from './media_interface';
import { VRInput, ButtonEvent, ButtonId } from './vr_input';
import * as THREE from 'three';

export class UserInterface {
  private renderer: THREE.WebGLRenderer;
  private scene: THREE.Scene;
  private vrInput: VRInput;
  private mediaInterface: MediaInterface;

  private group = new THREE.Group();
  private uiVisible = false;

  constructor(
    renderer: THREE.WebGLRenderer,
    scene: THREE.Scene,
    vrInput: VRInput,
    mediaInterface: MediaInterface,
  ) {
    this.renderer = renderer;
    this.scene = scene;
    this.vrInput = vrInput;
    this.mediaInterface = mediaInterface;

    this.vrInput.addEventListener('button', this.onButton.bind(this));

    this.createInterface();
    this.createWorldAxes();
    this.scene.add(this.group);
  }

  updateFrame() {
    // TODO
  }

  cleanupSession() {
    // TODO
  }

  private onButton(event: Event) {
    const ev = event as CustomEvent<ButtonEvent>;
    const buttonIndex = ev.detail.index;
    const pressed = ev.detail.pressed;

    // Trigger button to show/hide UI.
    if (!pressed && buttonIndex === ButtonId.TRIGGER) {
      this.toggleUi();
    }
  }

  private toggleUi() {
    this.uiVisible = !this.uiVisible;
    this.group.visible = this.uiVisible;
    if (this.uiVisible) this.repositionUi();
  }

  private repositionUi() {
    const camPos = new THREE.Vector3();
    const camDir = new THREE.Vector3();
    this.renderer.xr.getCamera().getWorldPosition(camPos);
    this.renderer.xr.getCamera().getWorldDirection(camDir);
    const uiDistance = 1.2;
    const uiPos = camPos.clone().addScaledVector(camDir, uiDistance);

    this.group.position.copy(uiPos);
    this.group.lookAt(camPos);
  }

  private createInterface() {
    // Shared material (semi-transparent gray)
    const material = new THREE.MeshBasicMaterial({
      color: 0x888888,
      transparent: true,
      opacity: 0.7,
      depthWrite: false, // better for UI
    });

    // Create planes.
    const planeWidth = 0.5;
    const planeHeight = 0.3;

    const geometry = new THREE.PlaneGeometry(planeWidth, planeHeight);

    const leftPlane = new THREE.Mesh(geometry, material);
    const centerPlane = new THREE.Mesh(geometry, material);
    const rightPlane = new THREE.Mesh(geometry, material);

    // Layout inside group.
    const spacing = 0.65;

    // Left (media selection)
    leftPlane.position.set(-spacing, 0, 0);

    // Center bottom (seek bar)
    centerPlane.position.set(0, -0.4, 0);

    // Right (actions)
    rightPlane.position.set(spacing, 0, 0);

    this.group.add(leftPlane);
    this.group.add(centerPlane);
    this.group.add(rightPlane);
  }

  private createWorldAxes() {
    const axesHelper = new THREE.AxesHelper(1.0);
    this.scene.add(axesHelper);
  }

}
