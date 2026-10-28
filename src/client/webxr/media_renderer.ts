import * as THREE from 'three';
import * as types from '../modules/client_types';
import { PhotoRenderer } from './photo_renderer';
import { SplatRenderer } from './splat_renderer'
import { VRInput } from './vr_input';

export class MediaRenderer {
  private renderer: THREE.WebGLRenderer;
  private scene: THREE.Scene;

  private photoRenderer: PhotoRenderer;
  private splatRenderer: SplatRenderer;

  constructor(renderer: THREE.WebGLRenderer, scene: THREE.Scene) {
    this.renderer = renderer;
    this.scene = scene;
    this.photoRenderer = new PhotoRenderer(this.renderer, this.scene);
    this.splatRenderer = new SplatRenderer(this.renderer, this.scene);
  }

  cleanupSession() {
    this.photoRenderer.changeMedia(undefined);
    this.splatRenderer.changeMedia(undefined);
  }

  changeMedia(media?: types.SelectedMedia) {
    if (!media) {
      this.cleanupSession();
      return;
    }

    const album = media.album;
    const entry = album.entries[media.index];
    if (entry.media === types.MediaType.IMAGE_GPHOTO ||
      entry.media === types.MediaType.IMAGE_SBS ||
      entry.media === types.MediaType.VIDEO_SBS) {
      this.splatRenderer.changeMedia(undefined);
      this.photoRenderer.changeMedia(media);
    } else if (entry.media === types.MediaType.GEOMETRY_SPLAT) {
      this.photoRenderer.changeMedia(undefined);
      this.splatRenderer.changeMedia(media);
    }
  }

  processInput(vrInput: VRInput) {
    this.splatRenderer.processInput(vrInput);
    this.photoRenderer.processInput(vrInput);
  }

  toggleVideoPause() {
    this.photoRenderer.toggleVideoPause();
  }

  toggleVideoMute() {
    this.photoRenderer.toggleVideoMute();
  }

  setSqueezeState(squeezeHeld: boolean) {
    this.photoRenderer.setSeekMode(squeezeHeld);
    this.splatRenderer.setMoveMode(squeezeHeld);
  }
}
