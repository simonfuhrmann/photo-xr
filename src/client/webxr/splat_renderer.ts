import * as THREE from 'three';
import * as types from '../modules/client_types';

export class SplatRenderer {
  private renderer: THREE.WebGLRenderer;
  private scene: THREE.Scene;

  constructor(renderer: THREE.WebGLRenderer, scene: THREE.Scene) {
    this.renderer = renderer;
    this.scene = scene;
  }

  changeMedia(media?: types.SelectedMedia) {
    this.cleanupResources();
    if (!media) return;
    // TODO
  }

  private cleanupResources() {
    // TODO
  }
}
