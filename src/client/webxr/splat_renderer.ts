import * as THREE from 'three';
import * as types from '../modules/client_types';
import { SparkRenderer, SplatMesh } from "@sparkjsdev/spark";
import * as stringUtils from '../modules/string_utils';
import { VRInput } from './vr_input';

export class SplatRenderer {
  private renderer: THREE.WebGLRenderer;
  private scene: THREE.Scene;
  private sparkRenderer?: SparkRenderer;
  private splatMesh?: SplatMesh;

  // In move mode, the controller transforms the splat.
  private moveMode = false;
  private baseRotation = new THREE.Quaternion(1, 0, 0, 0);
  private basePosition = new THREE.Vector3(0, 0, 0);
  private userPosition = new THREE.Vector3(0, 0, 0);

  constructor(renderer: THREE.WebGLRenderer, scene: THREE.Scene) {
    this.renderer = renderer;
    this.scene = scene;
    this.sparkRenderer = new SparkRenderer({
      renderer: this.renderer,
      // enableLod: true,
      // lodRenderScale: 5.0,
    });
  }

  changeMedia(media?: types.SelectedMedia) {
    this.cleanupResources();
    if (!media || !this.sparkRenderer) return;

    // Reset user transformations.
    this.userPosition = new THREE.Vector3(0, 0, 0);

    this.scene.add(this.sparkRenderer);
    this.splatMesh = new SplatMesh({
      url: this.getMediaUrl(media),
      // lod: true,
      // enableLod: true,
      onProgress: this.onProgress.bind(this),
      onLoad: this.onLoad.bind(this),
    });
  }

  processInput(vrInput: VRInput) {
    if (!this.splatMesh || !this.moveMode) return;

    // Get the max controller axes.
    const inputState = vrInput.getState();
    let axisY = 0.0;
    let axisX = 0.0;
    for (const [gamepad, state] of inputState) {
      if (Math.abs(state.axisY) > Math.abs(axisY)) axisY = state.axisY;
      if (Math.abs(state.axisX) > Math.abs(axisX)) axisX = state.axisX;
    }

    // Move the splat.
    const deltaTime = 1.0 / 60.0;  // TODO: Assume 60 fps.
    const distPerSec = 0.5;        // Max movement in meters per second.
    const moveY = -axisY * distPerSec * deltaTime;
    const oldPos = this.userPosition;
    this.userPosition.set(oldPos.x, oldPos.y + moveY, oldPos.z);
    this.splatMesh.position.set(
      this.basePosition.x + this.userPosition.x,
      this.basePosition.y + this.userPosition.y,
      this.basePosition.z + this.userPosition.z);
  }

  setMoveMode(move: boolean) {
    this.moveMode = move;
  }

  private cleanupResources() {
    if (this.splatMesh) {
      this.scene.remove(this.splatMesh);
      this.splatMesh.dispose();
    }
    if (this.sparkRenderer) {
      this.scene.remove(this.sparkRenderer);
    }
  }

  private onProgress(event: ProgressEvent) {
    const total = event.total;
    if (total == 0.0) return;
    const loaded = event.loaded;
    // console.log('progress', Math.round(loaded / total * 100.0));
  }

  private onLoad() {
    if (!this.splatMesh) return;
    console.log('mesh initialization complete');

    // Configuration.
    const maxSizeM = 0.5;  // Longest AABB side (in meters).
    const elevateM = 0.75;  // Elevate above ground (in meters).

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
    this.baseRotation.set(1, 0, 0, 0);
    this.splatMesh.quaternion.copy(this.baseRotation);

    // Position the rotated/scaled bounding-box center at (0, 0, -3), and
    // move the mesh up by some elevation, so it stands in front of the eyes.
    this.basePosition.set(
      -center.x * scale,
      (center.y + size.y / 2) * scale + elevateM,
      center.z * scale - 0.5,
    );
    this.splatMesh.position.copy(this.basePosition);

    this.scene.add(this.splatMesh);
  }

  private getMediaUrl(media: types.SelectedMedia) {
    // return "https://sparkjs.dev/assets/splats/butterfly.spz";
    const index = media.index;
    const album = media.album;
    const entry = album.entries[index];
    return stringUtils.joinPaths('/media', album.path, entry.name);
  }
}
