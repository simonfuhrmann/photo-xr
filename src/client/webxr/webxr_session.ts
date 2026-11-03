import * as THREE from 'three';
import * as types from '../modules/client_types';
import { VRInput, ButtonId, ButtonEvent, DirectionEvent, StickDir } from './vr_input';
import { MediaRenderer } from './media_renderer';
import { UserInterface } from './user_interface';
import { MediaInterface } from './media_interface';

/**
 * The WebXR session object that is created and disposed on demand when a WebXR
 * session is started and ended.
 */
export class WebXRSession {
  // THREE global objects.
  private scene: THREE.Scene;
  private renderer: THREE.WebGLRenderer;
  private camera: THREE.PerspectiveCamera;

  // VR input handling.
  private vrInput: VRInput;
  private squeezeHeld = false;

  // The album and media to display in this session.
  private mediaInterface = new MediaInterface();

  // The renderer for the eyes geometry.
  private mediaRenderer: MediaRenderer;

  // The renderer for the user interface.
  private userInterface: UserInterface;

  constructor() {
    this.scene = new THREE.Scene();
    this.scene.background = new THREE.Color(0x000000);
    this.camera = this.createCamera();

    // Set up the renderer and enable WebXR. Disable foveation since it causes
    // blurriness at the bottom of the rendering. Increasing the framebuffer
    // scale is expensive, but I imagine seeing a tiny difference. Need better
    // test images to see the difference.
    this.renderer = new THREE.WebGLRenderer({ antialias: false });
    this.renderer.setSize(window.innerWidth, window.innerHeight);
    this.renderer.xr.enabled = true;
    this.renderer.xr.setFoveation(0);
    this.renderer.xr.setFramebufferScaleFactor(1.5); // Test more.

    // Create the VR input infrastructure.
    this.vrInput = new VRInput(this.renderer, this.scene);
    this.vrInput.addControllerModels();
    this.vrInput.addEventListener('button', this.onButton.bind(this));
    this.vrInput.addEventListener('direction', this.onDirection.bind(this));

    this.mediaRenderer = new MediaRenderer(
      this.renderer, this.scene, this.vrInput, this.mediaInterface);
    this.userInterface = new UserInterface(
      this.renderer, this.scene, this.vrInput, this.mediaInterface);

    this.setCameraLayers();
  }

  setMedia(media: types.SelectedMedia) {
    this.mediaInterface.setMedia(media);
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
    this.mediaRenderer.cleanupSession();
    this.userInterface.cleanupSession();
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
    this.mediaRenderer.updateFrame();
    this.userInterface.updateFrame();
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
    // Enable the default layer (UI, controllers, splats).
    this.camera.layers.enable(0);

    // Enable the hemisphere layers for this camera (default is 0 only).
    // These layers show the left/right eye geometry for photo/video.
    this.camera.layers.enable(1);
    this.camera.layers.enable(2);
  }

  // TODO: Move input handling into MediaRenderer?
  private onButton(event: Event) {
    const ev = event as CustomEvent<ButtonEvent>;
    const buttonIndex = ev.detail.index;
    const pressed = ev.detail.pressed;

    // Squeeze button to start/stop splat movement.
    if (buttonIndex === ButtonId.SQUEEZE) {
      this.squeezeHeld = pressed;
      this.mediaRenderer.setSqueezeState(this.squeezeHeld);
    }
    // A-button to pause/unpause current video.
    if (pressed && buttonIndex === ButtonId.BUTTON_A) {
      this.mediaInterface.toggleVideoPause();
    }
    // B-button to mute/unmute current video.
    if (pressed && buttonIndex === ButtonId.BUTTON_B) {
      this.mediaInterface.toggleVideoMute();
    }
  }

  // TODO: Move input handling into MediaRenderer?
  private onDirection(event: Event) {
    if (this.squeezeHeld) return;

    const ev = event as CustomEvent<DirectionEvent>;
    const dir = ev.detail.direction;
    if (dir === StickDir.LEFT) this.mediaInterface.changeMedia(-1);
    if (dir === StickDir.RIGHT) this.mediaInterface.changeMedia(1);
  }
}
