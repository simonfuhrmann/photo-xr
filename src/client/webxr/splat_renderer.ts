import { MediaInterface } from './media_interface';
import { SparkRenderer, SplatMesh } from "@sparkjsdev/spark";
import { VRInput } from './vr_input';
import * as mediaUtils from '../modules/media_utils';
import * as THREE from 'three';
import * as types from '../modules/client_types';

export class SplatRenderer {
  private renderer: THREE.WebGLRenderer;
  private scene: THREE.Scene;
  private sparkRenderer: SparkRenderer;

  // A splat media is active if the splat mesh is defined.
  private splatMesh?: SplatMesh;
  private splatPivot = new THREE.Group();

  // In move mode, the controller transforms the splat.
  private vrInput: VRInput;
  private moveMode = false;
  private pivotRot = new THREE.Quaternion();
  private pivotPos = new THREE.Vector3();

  constructor(
    renderer: THREE.WebGLRenderer,
    scene: THREE.Scene,
    vrInput: VRInput,
    mediaInterface: MediaInterface,
  ) {
    this.renderer = renderer;
    this.scene = scene;
    this.vrInput = vrInput;

    this.scene.add(this.splatPivot);
    this.sparkRenderer = new SparkRenderer({
      renderer: this.renderer,
      // enableLod: true,
      // lodRenderScale: 5.0,
    });

    // Register media changed callback.
    mediaInterface.addMediaChangedCallback(() => {
      this.changeMedia(mediaInterface.getMedia());
    });
  }

  cleanupResources() {
    if (this.splatMesh) {
      this.splatPivot.remove(this.splatMesh);
      this.splatMesh.dispose();
      this.splatMesh = undefined;
    }
    if (this.sparkRenderer) {
      this.scene.remove(this.sparkRenderer);
    }
  }

  updateFrame() {
    this.processInputs();
  }

  // TODO: This should be removed and handled through MediaInterface.
  setMoveMode(move: boolean) {
    this.moveMode = move;
  }

  private changeMedia(media?: types.SelectedMedia) {
    this.cleanupResources();
    if (!media || !mediaUtils.isSplatMedia(media)) return;
  
    // Reset user transformations.
    this.pivotPos.set(0, 0.75, -0.5);
    this.pivotRot.set(0, 0, 0, 1);
    this.splatPivot.position.copy(this.pivotPos);
    this.splatPivot.quaternion.copy(this.pivotRot);

    // Load the splat configure a callback when loading finished.
    this.scene.add(this.sparkRenderer);
    this.splatMesh = new SplatMesh({
      url: mediaUtils.getMediaUrl(media),
      // lod: true,
      // enableLod: true,
      onProgress: this.onProgress.bind(this),
      onLoad: this.onLoad.bind(this),
    });
  }

  private processInputs() {
    if (!this.splatMesh || !this.moveMode) return;

    // Get the max controller axes.
    const inputState = this.vrInput.getState();
    let axisY = 0.0;
    let axisX = 0.0;
    for (const [gamepad, state] of inputState) {
      if (Math.abs(state.axisY) > Math.abs(axisY)) axisY = state.axisY;
      if (Math.abs(state.axisX) > Math.abs(axisX)) axisX = state.axisX;
    }

    // Deadzone for both movement and rotation.
    const magnitude = Math.max(Math.abs(axisX), Math.abs(axisY));
    if (magnitude < 0.1) return;

    // Move or rotate the splat, depending on the axis with larger magnitude.
    if (Math.abs(axisX) > Math.abs(axisY)) {
      this.rotateSplatPivot(axisX);
    } else {
      this.moveSplatPivot(axisY);
    }
  }

  private rotateSplatPivot(axis: number) {
    // Configuration.
    const deltaTime = 1.0 / 60.0;  // TODO: Assume 60 fps.
    const rotPerSec = 2.0;  // Max rotation in rad per second.

    // Rotate the splat.
    const rotRadY = axis * rotPerSec * deltaTime;
    const rotAxis = new THREE.Vector3(0, 1, 0);
    const rotY = new THREE.Quaternion().setFromAxisAngle(rotAxis, rotRadY);
    this.pivotRot.multiplyQuaternions(rotY, this.pivotRot);
    this.splatPivot.quaternion.copy(this.pivotRot);
  }

  private moveSplatPivot(axis: number) {
    // Configuration.
    const deltaTime = 1.0 / 60.0;  // TODO: Assume 60 fps.
    const distPerSec = 0.5;        // Max movement in meters per second.

    // Move the splat.
    const moveY = -axis * distPerSec * deltaTime;
    const pos = this.pivotPos;
    pos.set(pos.x, pos.y + moveY, pos.z);
    this.splatPivot.position.copy(pos);
  }

  private onProgress(event: ProgressEvent) {
    // const total = event.total;
    // if (total == 0.0) return;
    // const loaded = event.loaded;
    // console.log('progress', Math.round(loaded / total * 100.0));
  }

  private onLoad() {
    if (!this.splatMesh) return;
    console.log('mesh initialization complete');

    // Configuration.
    const maxSizeM = 0.5;  // Longest AABB side (in meters).

    // The order of scale, rotate and position calls is irrelevant. The final
    // transformation matrix is always computed as: T * R * S.
    // Further, SplatMesh.getBoundingBox() is performed on the (loaded) input
    // points and is unaffected by any transformations applied.
    const aabb = this.splatMesh.getBoundingBox();
    const center = aabb.min.clone().add(aabb.max).multiplyScalar(0.5);
    const size = aabb.max.clone().sub(aabb.min);
    const scale = maxSizeM / Math.max(size.x, size.y, size.z);

    // Scale the splat so the longest side has length 1.
    this.splatMesh.scale.setScalar(scale);

    // Perform a 180° rotation around X to account for the different coordinate 
    // system of WebGL and typical Splat software (Brush, SuperSplat, etc.).
    this.splatMesh.quaternion.set(1, 0, 0, 0);

    // Position the rotated/scaled AABB center at (0, h/2, 0). The h/2
    // moves the mesh up by half the AABB height, so it stands on the ground.
    this.splatMesh.position.set(
      -center.x * scale,
      (center.y + size.y / 2) * scale,
      center.z * scale,
    );

    this.splatPivot.add(this.splatMesh);
  }
}
