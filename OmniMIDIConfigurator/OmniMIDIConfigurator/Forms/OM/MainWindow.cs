﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using Un4seen.Bass;

namespace OmniMIDIConfigurator
{
    public partial class MainWindow : Form
    {
        public static class OMWC
        {
            public static SoundFontListEditor SFLE;
            public static SettingsPanel SET;
        }

        private bool CreateSFLEEmbed()
        {
            try
            {
                if (OMWC.SFLE != null)
                {
                    SFLEPanel.Controls.Remove(OMWC.SFLE);
                    OMWC.SFLE.Dispose();
                }

                OMWC.SFLE = new SoundFontListEditor();
                OMWC.SFLE.Dock = DockStyle.Fill;
                OMWC.SFLE.AutoScroll = false;
                SFLEPanel.Controls.Add(OMWC.SFLE);

                return true;
            }
            catch (Exception ex)
            {
                Program.ShowError(4, "SFLE Embed Error", "An error has occured while creating the SoundFonts list editor embed.", ex);
            }

            return false;
        }

        private bool CreateSETEmbed()
        {
            try
            {
                if (OMWC.SET != null)
                {
                    ApplySettings.Click -= new EventHandler(OMWC.SET.ButtonToSaveSettings);
                    RestoreDefault.Click -= new EventHandler(OMWC.SET.ButtonToResetSettings);

                    SETPanel.Controls.Remove(OMWC.SET);
                    OMWC.SET.Dispose();
                }

                OMWC.SET = new SettingsPanel();
                OMWC.SET.Dock = DockStyle.Fill;
                OMWC.SET.AutoScroll = false;
                OMWC.SET.HorizontalScroll.Enabled = false;
                OMWC.SET.HorizontalScroll.Visible = false;
                OMWC.SET.HorizontalScroll.Maximum = 0;
                OMWC.SET.AutoScroll = true;
                SETPanel.Controls.Add(OMWC.SET);

                QICombo.SelectedIndex = 0;

                ApplySettings.Click += new EventHandler(OMWC.SET.ButtonToSaveSettings);
                RestoreDefault.Click += new EventHandler(OMWC.SET.ButtonToResetSettings);

                return true;
            }
            catch (Exception ex)
            {
                Program.ShowError(4, "SET Embed Error", "An error has occured while creating the Settings panel embed.", ex);
            }

            return false;
        }

        public MainWindow()
        {
            // Initialize form
            InitializeComponent();

            // Check start location
            if (Properties.Settings.Default.LastWindowPos == new Point(-9999, -9999))
                this.StartPosition = FormStartPosition.CenterScreen;
            else
            {
                this.StartPosition = FormStartPosition.Manual;
                this.Location = Properties.Settings.Default.LastWindowPos;
            }

            // Check size
            if (Properties.Settings.Default.LastWindowSize != new Size(-1, -1))
                this.Size = Properties.Settings.Default.LastWindowSize;

            // Set menu
            Menu = OMMenu;

            // Check if MIDI mapper is available
            OMAPCpl.Visible = Functions.CheckMIDIMapper();

            // Add dynamic controls
            CreateSFLEEmbed();
            CreateSETEmbed();
        }

        // Wait for the resizing process to end before refreshing the window again
        protected override void OnResizeBegin(EventArgs e)
        {
            SuspendLayout();
            base.OnResizeBegin(e);
        }
        protected override void OnResizeEnd(EventArgs e)
        {
            ResumeLayout();
            base.OnResizeEnd(e);
        }
        // Wait for the resizing process to end before refreshing the window again

        protected override void WndProc(ref Message m)
        {
            base.WndProc(ref m);

            if (m.Msg == (int)Program.BringToFrontMessage)
            {
                WinAPI.ShowWindow(Handle, WinAPI.SW_RESTORE);
                WinAPI.SetForegroundWindow(Handle);
            }
        }

        private void MainWindow_ResizeEnd(object sender, EventArgs e)
        {
            Properties.Settings.Default.LastWindowPos = this.Location;
            Properties.Settings.Default.LastWindowSize = new Size(this.Size.Width, this.Size.Height - 21);
            Properties.Settings.Default.Save();
        }

        private void MainWindow_Load(object sender, EventArgs e)
        {
            // Nothing lul
        }

        private void DriverInfo_Click(object sender, EventArgs e)
        {
            new InfoWindow().ShowDialog();
        }

        private void QICombo_SelectedIndexChanged(object sender, EventArgs e)
        {
            switch (QICombo.SelectedIndex)
            {
                case 0:
                    OMWC.SET.ScrollControlIntoView(OMWC.SET.EnginesBox);
                    break;
                case 1:
                    OMWC.SET.ScrollControlIntoView(OMWC.SET.SynthBox);
                    break;
                case 2:
                    OMWC.SET.ScrollControlIntoView(OMWC.SET.MiscBox);
                    break;
                case 3:
                    OMWC.SET.ScrollControlIntoView(OMWC.SET.LegacySetDia);
                    break;
                default:
                    OMWC.SET.ScrollControlIntoView(OMWC.SET.EnginesBox);
                    break;
            }
        }

        private void OpenDW_Click(object sender, EventArgs e)
        {
            Process.Start(Environment.GetFolderPath(Environment.SpecialFolder.SystemX86) + "\\OmniMIDI\\OmniMIDIDebugWindow.exe");
        }

        private void OpenM_Click(object sender, EventArgs e)
        {
            Process.Start(Environment.GetFolderPath(Environment.SpecialFolder.SystemX86) + "\\OmniMIDI\\OmniMIDIMixerWindow.exe");
        }

        private void OpenBM_Click(object sender, EventArgs e)
        {
            new BlacklistSystem().ShowDialog();
        }

        private void OpenRTSSOSDM_Click(object sender, EventArgs e)
        {
            new RivaTunerSettings().ShowDialog();
        }

        private void CloseConfigurator_Click(object sender, EventArgs e)
        {
            try
            {
                Bass.BASS_Free();
                Application.Exit();
            }
            catch
            {
                Application.Exit();
            }
        }

        private void MainWindow_FormClosing(object sender, FormClosingEventArgs e)
        {
            CloseConfigurator_Click(null, null);
        }

        private void OMAPCpl_Click(object sender, EventArgs e)
        {
            new OmniMapperCpl().ShowDialog();
        }

        private void OMAPInstall_Click(object sender, EventArgs e)
        {
            Functions.MIDIMapRegistry(false);
            OMAPCpl.Visible = Functions.CheckMIDIMapper();
        }

        private void OMAPUninstall_Click(object sender, EventArgs e)
        {
            Functions.MIDIMapRegistry(true);
            OMAPCpl.Visible = Functions.CheckMIDIMapper();
        }

        private void InstallLM_Click(object sender, EventArgs e)
        {
            if (Convert.ToInt32(Program.SynthSettings.GetValue("AudioBitDepth", 1)) != 1)
            {
                DialogResult RES = Program.ShowError(3, "LoudMax", "LoudMax is useless without 32-bit float audio rendering.\nPlease enable it by going to \"Additional settings > Advanced audio settings > Audio bit depth\".\n\nDo you want to continue anyway?", null);
                if (RES == DialogResult.Yes) Functions.LoudMaxInstall();
            }
            else Functions.LoudMaxInstall();
        }

        private void UninstallLM_Click(object sender, EventArgs e)
        {
            Functions.LoudMaxUninstall();
        }

        private void WMWPatch_Click(object sender, EventArgs e)
        {
            new WinMMPatches().ShowDialog();
        }

        private void KACGuide_Click(object sender, EventArgs e)
        {
            Process.Start("https://github.com/KeppySoftware/OmniMIDI#how-can-i-get-rid-of-the-annoying-smartscreen-block-screen-and-stop-chrome-from-warning-me-not-to-download-your-driver");
        }

        private void MIDIInOutTest_Click(object sender, EventArgs e)
        {
            new MIDIInPlay().ShowDialog();
        }

        private void DeleteUserData_Click(object sender, EventArgs e)
        {
            DialogResult RES1 = Program.ShowError(3, "Clear user data", "Deleting the driver's user data will delete all the SoundFont lists, the DLL overrides and will also uninstall LoudMax.\nThis action is irreversible!\n\nAre you sure you want to continue?\nAfter deleting the data, the configurator will restart.", null);
            if (RES1 == DialogResult.Yes)
            {
                DialogResult RES2 = Program.ShowError(1, "Clear user data", "Would you like to restart the configurator after the process?", null);

                Functions.DeleteDirectory(System.Environment.GetEnvironmentVariable("USERPROFILE") + "\\OmniMIDI\\");

                if (RES2 == DialogResult.Yes)
                    System.Diagnostics.Process.Start(Application.ExecutablePath);

                this.Close();
            }
        }

        private void ReinstallDriver_Click(object sender, EventArgs e)
        {
            DialogResult RES = Program.ShowError(3, "Reinstall the driver", "Are you sure you want to reinstall the driver?\n\nThe configurator will download the latest installer, and remove all the old registry keys.\nYou'll lose ALL the settings.", null);
            if (RES == DialogResult.Yes)
            {
                var p = new System.Diagnostics.Process();
                p.StartInfo.FileName = System.Reflection.Assembly.GetEntryAssembly().Location;
                p.StartInfo.Arguments = "/REI";
                p.StartInfo.RedirectStandardOutput = true;
                p.StartInfo.UseShellExecute = false;
                p.StartInfo.CreateNoWindow = true;
                p.Start();
                Application.ExitThread();
            }
        }

        private void BugReport_Click(object sender, EventArgs e)
        {
            Process.Start("https://github.com/KeppySoftware/OmniMIDI/issues/");
        }

        private void CFUBtn_Click(object sender, EventArgs e)
        {
            if (Control.ModifierKeys == Keys.Shift) UpdateSystem.CheckForUpdates(true, false, false);
            else UpdateSystem.CheckForUpdates(false, false, false);
        }

        private void ChangelogCurrent_Click(object sender, EventArgs e)
        {
            try
            {
                FileVersionInfo Driver = FileVersionInfo.GetVersionInfo(UpdateSystem.UpdateFileVersion);
                new ChangelogWindow(Driver.FileVersion.ToString(), false).ShowDialog();
            }
            catch { }
        }

        private void ChangelogLatest_Click(object sender, EventArgs e)
        {
            try
            {
                Octokit.Release Release = UpdateSystem.UpdateClient.Repository.Release.GetLatest("KeppySoftware", "OmniMIDI").Result;
                Version x = null;
                Version.TryParse(Release.TagName, out x);
                new ChangelogWindow(x.ToString(), false).ShowDialog();
            }
            catch (Exception ex)
            {
                Program.ShowError(
                    4, 
                    "Error", 
                    "An error has occured while interrogating GitHub for the latest release.\n" +
                    "This is not a serious error, it might mean that your IP has reached the maximum requests allowed to the GitHub servers.",
                    ex);
            }
        }

        private void KDMAPIDoc_Click(object sender, EventArgs e)
        {
            Process.Start("https://github.com/KeppySoftware/OmniMIDI/blob/master/KDMAPI.md");
        }

        private void DLDriverSrc_Click(object sender, EventArgs e)
        {
            Process.Start("https://github.com/KeppySoftware/OmniMIDI");
        }
    }
}
