import * as THREE from 'three';
import * as types from '../modules/client_types';
import { VRInput } from './vr_input';
import { MediaViewer } from './media_viewer';

type ButtonEvent = { source: XRInputSource, button: number, pressed: boolean };

class PhotoXR {
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
    //this.vrInput.addControllerModels();
    this.vrInput.addEventListener('select', this.onSelect.bind(this));
    this.vrInput.addEventListener('squeeze', this.onSqueeze.bind(this));
    this.vrInput.addEventListener('button', this.onButton.bind(this));

    this.mediaViewer = new MediaViewer(this.renderer, this.scene);
  }

  setMedia(media: types.SelectedMedia) {
    this.media = media;
    this.onChangeMedia(0);
  }

  startSession(session: XRSession) {
    this.renderer.xr.setSession(session);
    this.renderer.setAnimationLoop(() => {
      const xrCameras = this.renderer.xr.getCamera();
      if (xrCameras.cameras.length === 2) {
        xrCameras.cameras[0].layers.set(1); // left eye
        xrCameras.cameras[1].layers.set(2); // right eye
      }
      this.vrInput.pollInputs();
      this.renderer.render(this.scene, this.camera);
    });
  }

  endSession() {
    this.renderer.xr.getSession()?.end();
  }

  cleanupSession() {
    this.renderer.xr.setSession(null);
    this.renderer.setAnimationLoop(null);
    this.renderer.dispose();
  }

  private createCamera(): THREE.PerspectiveCamera {
    // Create a camera. The FOV and aspect ratio is overridden by WebXR.
    const aspect = window.innerWidth / window.innerHeight;
    const zNear = 0.1;
    const zFar = 100.0;
    const camera = new THREE.PerspectiveCamera(/*fov=*/70, aspect, zNear, zFar);

    // Enable the hemisphere layers for this camera (default is 0 only).
    camera.layers.enable(1);
    camera.layers.enable(2);
    return camera;
  }

  private onSelect(event: Event) {
    // This will bring up the immersive UI.
    console.log('select event');
  }

  private onSqueeze(event: Event) {
    console.log('squeeze event', event);
  }

  private onButton(event: Event) {
    const ev = event as CustomEvent<ButtonEvent>;
    const buttonIndex = ev.detail.button;
    const pressed = ev.detail.pressed;
    if (!pressed) return;
    if (buttonIndex === 4) this.onChangeMedia(1);
    if (buttonIndex === 5) this.onChangeMedia(-1);
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

// Singleton instance, created and disposed on demand.
let photoXR: PhotoXR | undefined;

function cleanupSession() {
  photoXR?.cleanupSession();
  photoXR = undefined;
  console.log('XR session ended');
}

// Initializes WebXR and starts the session.
export function startSession(session: XRSession, media: types.SelectedMedia) {
  if (!!photoXR) return;
  photoXR = new PhotoXR();
  photoXR.startSession(session);
  photoXR.setMedia(media);
  console.log('XR session started');
  session.addEventListener('end', cleanupSession);
}

// Requets to ends the WebXR session.
export function endSession() {
  photoXR?.endSession();
}

export function updateMedia(media: types.SelectedMedia) {
  if (!photoXR) return;
  photoXR.setMedia(media);
}
