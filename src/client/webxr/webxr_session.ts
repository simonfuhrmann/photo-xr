import * as THREE from 'three';
import * as types from '../modules/client_types';
import { VRInput, ButtonEvent, DirectionEvent, StickDir } from './vr_input';
import { MediaViewer } from './media_viewer';
import { UserInterface } from './user_interface';

/**
 * The WebXR session object that is created and disposed on demand when a WebXR
 * session is started and ended.
 */
export class WebXRSession {
  // The album and media to display in this session.
  private media?: types.SelectedMedia;

  // THREE global objects.
  private scene: THREE.Scene;
  private renderer: THREE.WebGLRenderer;
  private camera: THREE.PerspectiveCamera;

  // VR input handling.
  private vrInput: VRInput;

  // The renderer for the eyes geometry.
  private mediaViewer: MediaViewer;

  // The renderer for the user interface.
  private userInterface: UserInterface;

  constructor() {
    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0x000000);
    this.camera = this.createCamera();

    // Set up the renderer and enable WebXR. Disable foveation since it causes
    // blurriness at the bottom of the rendering. Increasing the framebuffer
    // scale is expensive, but I imagine to see a tiny difference. Need better
    // test images to see the difference.
    this.renderer = new THREE.WebGLRenderer({ antialias: false });
    this.renderer.setSize(window.innerWidth, window.innerHeight);
    this.renderer.xr.enabled = true;
    this.renderer.xr.setFoveation(0);
    this.renderer.xr.setFramebufferScaleFactor(1.5); // Test more.

    // Create the VR input infrastructure.
    this.vrInput = new VRInput(this.renderer, this.scene);
    this.vrInput.addControllerModels();
    this.vrInput.addEventListener('select', this.onSelect.bind(this));
    this.vrInput.addEventListener('squeeze', this.onSqueeze.bind(this));
    this.vrInput.addEventListener('button', this.onButton.bind(this));
    this.vrInput.addEventListener('direction', this.onDirection.bind(this));

    this.mediaViewer = new MediaViewer(this.renderer, this.scene);
    this.userInterface = new UserInterface(this.renderer, this.scene);

    this.setCameraLayers();
  }

  setMedia(media: types.SelectedMedia) {
    this.media = media;
    this.onChangeMedia(0);
  }

  startSession(session: XRSession) {
    this.renderer.xr.setSession(session);
    this.renderer.setAnimationLoop(this.animationLoop.bind(this));
  }

  // Tiggers a request to end the session. The session fires the 'end' event
  // when it is actually ended, which triggers cleanup.
  endSession() {
    this.renderer.xr.getSession()?.end();
  }

  cleanupSession() {
    this.renderer.xr.setSession(null);
    this.renderer.setAnimationLoop(null);
    this.renderer.dispose();
  }

  private animationLoop() {
    const xrCameras = this.renderer.xr.getCamera();
    if (xrCameras.cameras.length === 2) {
      xrCameras.cameras[0].layers.enable(1); // left eye
      xrCameras.cameras[1].layers.enable(2); // right eye
    }
    this.vrInput.pollInputs();
    this.renderer.render(this.scene, this.camera);
  }

  private createCamera(): THREE.PerspectiveCamera {
    // Create a camera. The FOV and aspect ratio is overridden by WebXR.
    const aspect = window.innerWidth / window.innerHeight;
    const zNear = 0.1;
    const zFar = 100.0;
    const camera = new THREE.PerspectiveCamera(/*fov=*/70, aspect, zNear, zFar);
    return camera;
  }

  private setCameraLayers() {
    // Enable the hemisphere layers for this camera (default is 0 only).
    this.camera.layers.enable(1);
    this.camera.layers.enable(2);
    if (this.userInterface.getUiVisible()) {
      this.camera.layers.enable(0);
    } else {
      this.camera.layers.disable(0);
    }
  }

  // Called when the trigger is pressed, brings up the immersive UI.
  private onSelect(event: Event) {
    this.userInterface.toggleUi();
    this.setCameraLayers();
  }

  // Called when the squeeze button is pressed, currently does nothing.
  private onSqueeze(event: Event) {
    console.log('squeeze event', event);
  }

  private onButton(event: Event) {
    const ev = event as CustomEvent<ButtonEvent>;
    const buttonIndex = ev.detail.button;
    const pressed = ev.detail.pressed;
    if (!pressed) return;
    // A-button is 4 (pause current video), B-button is 5 (no-op).
    if (buttonIndex === 4) {
      this.mediaViewer.toggleVideoPause();
    }
  }

  private onDirection(event: Event) {
    const ev = event as CustomEvent<DirectionEvent>;
    const dir = ev.detail.direction;
    if (dir === StickDir.LEFT) this.onChangeMedia(-1);
    if (dir === StickDir.RIGHT) this.onChangeMedia(1);
  }

  private onChangeMedia(delta: number) {
    if (!this.media) return;

    // Advance the media index.
    const album = this.media.album;
    const count = album.entries.length;
    this.media.index = (this.media.index + delta + count) % count;
    this.mediaViewer.changeMedia(this.media);
  }
}
