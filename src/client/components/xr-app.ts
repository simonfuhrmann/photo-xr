import { html, css, LitElement } from 'lit';
import { customElement } from 'lit/decorators.js';

@customElement('xr-app')
export class XrApp extends LitElement {
  static styles = css`
    :host {
      width: 100vw;
      height: 100vh;
      display: flex;
    }
  `;

  override render() {
    return html`
      <h1>PhotoXR - A WebXR Photo Viewer</h1>
    `;
  }
}
