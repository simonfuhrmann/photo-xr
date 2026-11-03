import { MediaInterface } from './media_interface';
import { PhotoRenderer } from './photo_renderer';
import { SplatRenderer } from './splat_renderer'
import { VRInput } from './vr_input';
import * as THREE from 'three';

export class MediaRenderer {
  private photoRenderer: PhotoRenderer;
  private splatRenderer: SplatRenderer;

  constructor(
    renderer: THREE.WebGLRenderer,
    scene: THREE.Scene,
    mediaInterface: MediaInterface,
  ) {
    this.photoRenderer = new PhotoRenderer(renderer, scene, mediaInterface);
    this.splatRenderer = new SplatRenderer(renderer, scene, mediaInterface);
  }

  cleanupSession() {
    this.photoRenderer.cleanupResources();
    this.splatRenderer.cleanupResources();
  }

  updateFrame(vrInput: VRInput) {
    this.splatRenderer.updateFrame(vrInput);
    this.photoRenderer.updateFrame(vrInput);
  }

  setSqueezeState(squeezeHeld: boolean) {
    this.photoRenderer.setSeekMode(squeezeHeld);
    this.splatRenderer.setMoveMode(squeezeHeld);
  }
}
