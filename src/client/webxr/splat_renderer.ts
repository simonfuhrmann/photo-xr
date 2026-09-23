import * as THREE from 'three';
import * as types from '../modules/client_types';
import { SparkRenderer, SplatMesh } from "@sparkjsdev/spark";
import * as stringUtils from '../modules/string_utils';

export class SplatRenderer {
  private renderer: THREE.WebGLRenderer;
  private scene: THREE.Scene;
  private sparkRenderer?: SparkRenderer;
  private splatMesh?: SplatMesh;

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

    this.scene.add(this.sparkRenderer);
    this.splatMesh = new SplatMesh({
      url: this.getMediaUrl(media),
      // lod: true,
      // enableLod: true,
      onProgress: this.onProgress.bind(this),
      onLoad: this.onLoad.bind(this),
    });
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
    this.splatMesh.quaternion.set(1, 0, 0, 0);

    // Position the rotated/scaled bounding-box center at (0, 0, -3), and
    // move the mesh up by half the AABB height, so it stands on the ground.
    this.splatMesh.position.set(
      -center.x * scale,
      (center.y + size.y / 2) * scale + elevateM,
      center.z * scale - 0.5,
    );

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
