import * as types from '../modules/client_types';
import { WebXRSession } from './webxr_session';

/**
 * The entry point for the web client to start the XR session. This class
 * manages the lifecycle of the XR session and exports a singleton interface.
 */
export class WebXR extends EventTarget {
  private session?: WebXRSession;

  startSession(session: XRSession, media: types.SelectedMedia) {
    if (!!this.session) return;
    this.session = new WebXRSession();
    this.session.startSession(session);
    this.session.setMedia(media);
    session.addEventListener('end', this.cleanupSession.bind(this));

    console.log('XR session started');
    this.dispatchEvent(new CustomEvent('session-started'));
  }

  endSession() {
    this.session?.endSession();
  }

  updateMedia(media: types.SelectedMedia) {
    if (!this.session) return;
    this.session.setMedia(media);
  }

  private cleanupSession() {
    this.session?.cleanupSession();
    this.session = undefined;

    console.log('XR session ended');
    this.dispatchEvent(new CustomEvent('session-ended'));
  }
}

let webXR = new WebXR();
export { webXR };
