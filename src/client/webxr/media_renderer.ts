import { MediaInterface } from './media_interface';
import { PhotoRenderer } from './photo_renderer';
import { VideoRenderer } from './video_renderer';
import { SplatRenderer } from './splat_renderer'
import { VRInput } from './vr_input';
import * as THREE from 'three';

export class MediaRenderer {
  private photoRenderer: PhotoRenderer;
  private videoRenderer: VideoRenderer;
  private splatRenderer: SplatRenderer;

  constructor(
    renderer: THREE.WebGLRenderer,
    scene: THREE.Scene,
    vrInput: VRInput,
    mediaInterface: MediaInterface,
  ) {
    this.photoRenderer = new PhotoRenderer(
      renderer, scene, vrInput, mediaInterface);
    this.videoRenderer = new VideoRenderer(
      renderer, scene, vrInput, mediaInterface);
    this.splatRenderer = new SplatRenderer(
      renderer, scene, vrInput, mediaInterface);
  }

  cleanupSession() {
    this.photoRenderer.cleanupResources();
    this.videoRenderer.cleanupResources();
    this.splatRenderer.cleanupResources();
  }

  updateFrame() {
    this.photoRenderer.updateFrame();
    this.videoRenderer.updateFrame();
    this.splatRenderer.updateFrame();
  }

  setSqueezeState(squeezeHeld: boolean) {
    this.videoRenderer.setSeekMode(squeezeHeld);
    this.splatRenderer.setMoveMode(squeezeHeld);
  }
}
