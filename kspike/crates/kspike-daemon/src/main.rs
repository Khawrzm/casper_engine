//! kspiked binary.

use anyhow::Result;
use clap::Parser;
use kspike_core::BANNER;
use kspike_daemon::Daemon;
use std::path::PathBuf;
// Signal handling is Unix-specific, we will conditionally compile it.
#[cfg(unix)]
use tokio::signal::unix::{signal, SignalKind};

#[derive(Parser)]
#[command(name = "kspiked", version, about = "KSpike daemon")]
struct Cli {
    /// Socket path (used on Unix, ignored on Windows).
    #[arg(long, default_value = "/run/kspike.sock")]
    socket: PathBuf,
    /// Ledger file.
    #[arg(long, default_value = "/var/lib/kspike/ledger.jsonl")]
    ledger: PathBuf,
    /// KHZ Φ threshold.
    #[arg(long, default_value_t = 0.50)]
    phi: f32,
    /// Never apply — evaluate + judge + ledger only.
    #[arg(long)]
    dry_run: bool,
}

#[tokio::main]
async fn main() -> Result<()> {
    tracing_subscriber::fmt()
        .with_env_filter(tracing_subscriber::EnvFilter::from_default_env()
            .add_directive("info".parse().unwrap()))
        .with_target(false).compact().init();
    println!("{BANNER}");

    let cli = Cli::parse();
    let daemon = Daemon::new(cli.socket.clone(), Some(cli.ledger.clone()), cli.phi, cli.dry_run)?;
    let shutdown = daemon.shutdown_handle();

    // On Windows, we'll rely on Ctrl-C. On Unix, we handle signals gracefully.
    #[cfg(unix)]
    {
        let sd = shutdown.clone();
        tokio::spawn(async move {
            let mut term = signal(SignalKind::terminate()).unwrap();
            let mut intr = signal(SignalKind::interrupt()).unwrap();
            tokio::select! {
                _ = term.recv() => {}, _ = intr.recv() => {},
            }
            sd.notify_one();
        });
    }
    // On Windows, this allows Ctrl-C to trigger a shutdown.
    #[cfg(windows)]
    {
        let sd = shutdown.clone();
        tokio::spawn(async move {
            let _ = tokio::signal::ctrl_c().await;
            sd.notify_one();
        });
    }


    daemon.serve().await?;
    Ok(())
}
